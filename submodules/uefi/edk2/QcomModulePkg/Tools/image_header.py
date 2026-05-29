 # Copyright (c) 2015, The Linux Foundation. All rights reserved.
 #
 # Redistribution and use in source and binary forms, with or without
 # modification, are permitted provided that the following conditions are
 # met:
 # * Redistributions of source code must retain the above copyright
 #  notice, this list of conditions and the following disclaimer.
 #  * Redistributions in binary form must reproduce the above
 # copyright notice, this list of conditions and the following
 # disclaimer in the documentation and/or other materials provided
 #  with the distribution.
 #   * Neither the name of The Linux Foundation nor the names of its
 # contributors may be used to endorse or promote products derived
 # from this software without specific prior written permission.
 #
 # THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 # WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 # MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 # ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 # BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 # CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 # SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 # BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 # WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 # OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 # IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
from __future__  import print_function
import os
import sys
import elf_tools

if len(sys.argv) < 5:
   print ("Incorrect Usage: image_header.py <source file> <target file> <image_destination> <mbn type> <opt: elf_type>")
   print (r"Example: image_header.py ..\Bin64\ABL.fv ..\Bin64\unsigned\ABL.fv.elf 0x98100000 elf 64")
   raise RuntimeError("Usage: image_header.py <source file> <target file> <image_destination> <mbn type> <opt: elf_type>" )

gen_dict = {}
source_full = sys.argv[1]
target_full = sys.argv[2]
# Attempt to evaluate value
try:
   image_destination = eval(sys.argv[3])
# Catch exceptions and do not evaluate
except:
   raise RuntimeError("Invalid image destination address")

mbn_type = sys.argv[4]

is_elf_64_bit = False
no_hash_seg = False
if len(sys.argv) >= 6:
   if sys.argv[5] == '64':
      is_elf_64_bit = True
if len(sys.argv) >= 7:
   if sys.argv[6] == 'nohash':
      no_hash_seg = True
source_base = os.path.splitext(str(source_full))[0]
target_base = os.path.splitext(str(source_full))[0]

header_format = 'reg'
gen_dict['IMAGE_KEY_IMAGE_ID'] = elf_tools.ImageType.APPSBL_IMG
gen_dict['IMAGE_KEY_IMAGE_SOURCE'] = 0
gen_dict['IMAGE_KEY_IMAGE_DEST'] = image_destination
gen_dict['IMAGE_KEY_MBN_TYPE'] = mbn_type
image_header_secflag = 'non_secure'
image_size = os.stat(source_full).st_size

#----------------------------------------------------------------------------
# Generate UEFI mbn
#----------------------------------------------------------------------------

if mbn_type == 'elf':

    source_elf = source_base + ".elf"
    target_hash = target_base + ".hash"
    target_hash_hd = target_base + "_hash.hd"
    target_phdr_elf = target_base + "_phdr.pbn"
    target_nonsec = target_base + "_combined_hash.mbn"

    # Create elf header for UEFI
    rv = elf_tools.create_elf_header(target_base + ".hd", image_destination, image_size, is_elf_64_bit = is_elf_64_bit)
    if rv:
       raise RuntimeError("Failed to create elf header")

    files_to_cat_in_order = [target_base + ".hd", source_full]
    if no_hash_seg == False:
      elf_tools.concat_files (source_elf, files_to_cat_in_order)

      # Create hash table
      rv = elf_tools.pboot_gen_elf([], source_elf, target_hash,
                                   elf_out_file_name = target_phdr_elf,
                                   secure_type = image_header_secflag)
      if rv:
         raise RuntimeError("Failed to run pboot_gen_elf")

      # Create hash table header
      rv = elf_tools.image_header(os.environ, gen_dict, target_hash, target_hash_hd,
                           image_header_secflag, elf_file_name = source_elf)
      if rv:
         raise RuntimeError("Failed to create image header for hash segment")

      files_to_cat_in_order = [target_hash_hd, target_hash]
      elf_tools.concat_files (target_nonsec, files_to_cat_in_order)

      # Add the hash segment into the ELF
      elf_tools.pboot_add_hash([], target_phdr_elf, target_nonsec, target_full)
    else:
      # Generate non hash segment ELF file
      elf_tools.concat_files (source_elf, files_to_cat_in_order)
      rv = elf_tools.pboot_gen_elf([], source_elf, None,
                                   elf_out_file_name = target_full,
                                   secure_type = image_header_secflag)
      if rv:
         raise RuntimeError("Failed to run pboot_gen_elf")

