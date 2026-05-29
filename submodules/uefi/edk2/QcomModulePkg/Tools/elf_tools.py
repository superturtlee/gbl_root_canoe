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

import sys
import struct


#----------------------------------------------------------------------------
# GLOBAL VARIABLES BEGIN
#----------------------------------------------------------------------------
CERT_CHAIN_ONEROOT_MAXSIZE = 6*1024          # Default Cert Chain Max Size for one root


# ELF Common Definitions
ELF_HDR_COMMON_SIZE       = 24
ELFINFO_MAG0_INDEX        = 0
ELFINFO_MAG1_INDEX        = 1
ELFINFO_MAG2_INDEX        = 2
ELFINFO_MAG3_INDEX        = 3
ELFINFO_MAG0              = b'\x7f'
ELFINFO_MAG1              = b'E'
ELFINFO_MAG2              = b'L'
ELFINFO_MAG3              = b'F'
ELFINFO_CLASS_INDEX       = 4
ELFINFO_CLASS_32          = b'\x01'
ELFINFO_CLASS_64          = b'\x02'
ELFINFO_VERSION_INDEX     = 6
ELFINFO_VERSION_CURRENT   = b'\x01'
ELF_BLOCK_ALIGN           = 0x1000
ELFINFO_DATA2LSB          = b'\x01'
ELFINFO_EXEC_ETYPE        = b'\x02\x00'
ELFINFO_ARM_MACHINETYPE   = b'\x28\x00'
ELFINFO_VERSION_EV_CURRENT = b'\x01\x00\x00\x00'
ELFINFO_SHOFF             = 0x00
ELFINFO_PHNUM             = b'\x01\x00'
ELFINFO_RESERVED          = 0x00

# ELF 64 bit definitions
ELF64_HDR_SIZE            = 64
ELF64_PHDR_SIZE           = 56

# ELF 32 bit definitions
ELF32_HDR_SIZE            = 52
ELF32_PHDR_SIZE           = 32

MAX_PHDR_COUNT            = 100             # Maximum allowable program headers

# ELF Program Header Types
NULL_TYPE                 = 0x0
LOAD_TYPE                 = 0x1
DYNAMIC_TYPE              = 0x2
INTERP_TYPE               = 0x3
NOTE_TYPE                 = 0x4
SHLIB_TYPE                = 0x5
PHDR_TYPE                 = 0x6
TLS_TYPE                  = 0x7

"""
The eight bits between 20 and 27 in the p_flags field in ELF program headers
is not used by the standard ELF format. We use this byte to hold OS and processor
specific fields as recommended by ARM.

The bits in this byte are defined as follows:

                   Pool Indx    Segment type     Access type    Page/non page
  bits in p_flags /-----27-----/----26-24-------/---- 23-21----/------20-------/

After parsing segment description strings in the SCL file, the appropriate segment
flag values are chosen from the follow definitions. The mask defined below is then
used to update the existing p_flags field in the program headers with the updated
values.
"""
# Mask for bits 20-27 to parse program header p_flags
MI_PBT_FLAGS_MASK = 0x0FF00000

# Helper defines to help parse ELF program headers
MI_PROG_BOOT_DIGEST_SIZE              = 20
MI_PBT_FLAG_SEGMENT_TYPE_MASK         = 0x07000000
MI_PBT_FLAG_SEGMENT_TYPE_SHIFT        = 0x18
MI_PBT_FLAG_PAGE_MODE_MASK            = 0x00100000
MI_PBT_FLAG_PAGE_MODE_SHIFT           = 0x14
MI_PBT_FLAG_ACCESS_TYPE_MASK          = 0x00E00000
MI_PBT_FLAG_ACCESS_TYPE_SHIFT         = 0x15
MI_PBT_FLAG_POOL_INDEX_MASK           = 0x08000000
MI_PBT_FLAG_POOL_INDEX_SHIFT          = 0x1B

# Segment Type
MI_PBT_L4_SEGMENT                     = 0x0
MI_PBT_AMSS_SEGMENT                   = 0x1
MI_PBT_HASH_SEGMENT                   = 0x2
MI_PBT_BOOT_SEGMENT                   = 0x3
MI_PBT_L4BSP_SEGMENT                  = 0x4
MI_PBT_SWAPPED_SEGMENT                = 0x5
MI_PBT_SWAP_POOL_SEGMENT              = 0x6
MI_PBT_PHDR_SEGMENT                   = 0x7

# Page/Non-Page Type
MI_PBT_NON_PAGED_SEGMENT              = 0x0
MI_PBT_PAGED_SEGMENT                  = 0x1

# Access Type
MI_PBT_RW_SEGMENT                     = 0x0
MI_PBT_RO_SEGMENT                     = 0x1
MI_PBT_ZI_SEGMENT                     = 0x2
MI_PBT_NOTUSED_SEGMENT                = 0x3
MI_PBT_SHARED_SEGMENT                 = 0x4
MI_PBT_RWE_SEGMENT                    = 0x7

# ELF Segment Flag Definitions
MI_PBT_ELF_AMSS_NON_PAGED_RO_SEGMENT  = 0x01200000
MI_PBT_ELF_AMSS_PAGED_RO_SEGMENT  = 0x01300000
MI_PBT_ELF_SWAP_POOL_NON_PAGED_ZI_SEGMENT_INDEX0 = 0x06400000
MI_PBT_ELF_SWAPPED_PAGED_RO_SEGMENT_INDEX0 = 0x05300000
MI_PBT_ELF_SWAP_POOL_NON_PAGED_ZI_SEGMENT_INDEX1 = 0x0E400000
MI_PBT_ELF_SWAPPED_PAGED_RO_SEGMENT_INDEX1 = 0x0D300000
MI_PBT_ELF_AMSS_NON_PAGED_ZI_SEGMENT = 0x01400000
MI_PBT_ELF_AMSS_PAGED_ZI_SEGMENT = 0x01500000
MI_PBT_ELF_AMSS_NON_PAGED_RW_SEGMENT = 0x01000000
MI_PBT_ELF_AMSS_PAGED_RW_SEGMENT = 0x01100000
MI_PBT_ELF_AMSS_NON_PAGED_NOTUSED_SEGMENT = 0x01600000
MI_PBT_ELF_AMSS_PAGED_NOTUSED_SEGMENT = 0x01700000
MI_PBT_ELF_AMSS_NON_PAGED_SHARED_SEGMENT = 0x01800000
MI_PBT_ELF_AMSS_PAGED_SHARED_SEGMENT = 0x01900000
MI_PBT_ELF_HASH_SEGMENT = 0x02200000
MI_PBT_ELF_BOOT_SEGMENT = 0x03200000
MI_PBT_ELF_PHDR_SEGMENT = 0x07000000
MI_PBT_ELF_NON_PAGED_L4BSP_SEGMENT = 0x04000000
MI_PBT_ELF_PAGED_L4BSP_SEGMENT = 0x04100000
MI_PBT_ELF_AMSS_RELOCATABLE_IMAGE = 0x8000000

#----------------------------------------------------------------------------
# GLOBAL VARIABLES END
#----------------------------------------------------------------------------

#----------------------------------------------------------------------------
# CLASS DEFINITIONS BEGIN
#----------------------------------------------------------------------------
#----------------------------------------------------------------------------
# Image Type ID Class - These values must be kept consistent with mibib.h
#----------------------------------------------------------------------------
class ImageType:
   NONE_IMG           = 0
   APPSBL_IMG         = 5

#----------------------------------------------------------------------------
# Header Class Notes:
# In order to properly read and write the header structures as binary data,
# the Python Struct library is used to align and package up the header objects
# All Struct objects are initialized by a special string with the following
# notation. These structure objects are then used to decode binary data in order
# to fill out the appropriate class in Python, or they are used to package up
# the Python class so that we may write the binary data out.
#----------------------------------------------------------------------------
"""
      Format | C Type         | Python Type | Standard Size
      -----------------------------------------------------
    1) 'X's  | char *         | string      | 'X' bytes
    2) H     | unsigned short | integer     | 2 bytes
    3) I     | unsigned int   | integer     | 4 bytes
    4) Q     | unsigned long  | integer     | 8 bytes
"""

#----------------------------------------------------------------------------
# ELF Header Class
#----------------------------------------------------------------------------
class Elf_Ehdr_common:
   # Structure object to align and package the ELF Header
   s = struct.Struct('16sHHI')

   def __init__(self, data):
      unpacked_data       = (Elf_Ehdr_common.s).unpack(data)
      self.unpacked_data  = unpacked_data
      self.e_ident        = unpacked_data[0]
      self.e_type         = unpacked_data[1]
      self.e_machine      = unpacked_data[2]
      self.e_version      = unpacked_data[3]

   def printValues(self):
      print("ATTRIBUTE / VALUE")
      for attr, value in list(self.__dict__.items()):
         print((attr, value))


#----------------------------------------------------------------------------
# ELF Header Class
#----------------------------------------------------------------------------
class Elf32_Ehdr:
   # Structure object to align and package the ELF Header
   s = struct.Struct('16sHHIIIIIHHHHHH')

   def __init__(self, data):
      unpacked_data       = (Elf32_Ehdr.s).unpack(data)
      self.unpacked_data  = unpacked_data
      self.e_ident        = unpacked_data[0]
      self.e_type         = unpacked_data[1]
      self.e_machine      = unpacked_data[2]
      self.e_version      = unpacked_data[3]
      self.e_entry        = unpacked_data[4]
      self.e_phoff        = unpacked_data[5]
      self.e_shoff        = unpacked_data[6]
      self.e_flags        = unpacked_data[7]
      self.e_ehsize       = unpacked_data[8]
      self.e_phentsize    = unpacked_data[9]
      self.e_phnum        = unpacked_data[10]
      self.e_shentsize    = unpacked_data[11]
      self.e_shnum        = unpacked_data[12]
      self.e_shstrndx     = unpacked_data[13]

   def printValues(self):
      print("ATTRIBUTE / VALUE")
      for attr, value in list(self.__dict__.items()):
         print((attr, value))

   def getPackedData(self):
      values = [self.e_ident,
                self.e_type,
                self.e_machine,
                self.e_version,
                self.e_entry,
                self.e_phoff,
                self.e_shoff,
                self.e_flags,
                self.e_ehsize,
                self.e_phentsize,
                self.e_phnum,
                self.e_shentsize,
                self.e_shnum,
                self.e_shstrndx
               ]

      return (Elf32_Ehdr.s).pack(*values)

#----------------------------------------------------------------------------
# ELF Program Header Class
#----------------------------------------------------------------------------
class Elf32_Phdr:

   # Structure object to align and package the ELF Program Header
   s = struct.Struct('I' * 8)

   def __init__(self, data):
      unpacked_data       = (Elf32_Phdr.s).unpack(data)
      self.unpacked_data  = unpacked_data
      self.p_type         = unpacked_data[0]
      self.p_offset       = unpacked_data[1]
      self.p_vaddr        = unpacked_data[2]
      self.p_paddr        = unpacked_data[3]
      self.p_filesz       = unpacked_data[4]
      self.p_memsz        = unpacked_data[5]
      self.p_flags        = unpacked_data[6]
      self.p_align        = unpacked_data[7]

   def printValues(self):
      print("ATTRIBUTE / VALUE")
      for attr, value in list(self.__dict__.items()):
         print((attr, value))

   def getPackedData(self):
      values = [self.p_type,
                self.p_offset,
                self.p_vaddr,
                self.p_paddr,
                self.p_filesz,
                self.p_memsz,
                self.p_flags,
                self.p_align
               ]

      return (Elf32_Phdr.s).pack(*values)

#----------------------------------------------------------------------------
# ELF Header Class
#----------------------------------------------------------------------------
class Elf64_Ehdr:
   # Structure object to align and package the ELF Header
   s = struct.Struct('16sHHIQQQIHHHHHH')

   def __init__(self, data):
      unpacked_data       = (Elf64_Ehdr.s).unpack(data)
      self.unpacked_data  = unpacked_data
      self.e_ident        = unpacked_data[0]
      self.e_type         = unpacked_data[1]
      self.e_machine      = unpacked_data[2]
      self.e_version      = unpacked_data[3]
      self.e_entry        = unpacked_data[4]
      self.e_phoff        = unpacked_data[5]
      self.e_shoff        = unpacked_data[6]
      self.e_flags        = unpacked_data[7]
      self.e_ehsize       = unpacked_data[8]
      self.e_phentsize    = unpacked_data[9]
      self.e_phnum        = unpacked_data[10]
      self.e_shentsize    = unpacked_data[11]
      self.e_shnum        = unpacked_data[12]
      self.e_shstrndx     = unpacked_data[13]

   def printValues(self):
      print("ATTRIBUTE / VALUE")
      for attr, value in list(self.__dict__.items()):
         print((attr, value))

   def getPackedData(self):
      values = [self.e_ident,
                self.e_type,
                self.e_machine,
                self.e_version,
                self.e_entry,
                self.e_phoff,
                self.e_shoff,
                self.e_flags,
                self.e_ehsize,
                self.e_phentsize,
                self.e_phnum,
                self.e_shentsize,
                self.e_shnum,
                self.e_shstrndx
               ]

      return (Elf64_Ehdr.s).pack(*values)

#----------------------------------------------------------------------------
# ELF Program Header Class
#----------------------------------------------------------------------------
class Elf64_Phdr:

   # Structure object to align and package the ELF Program Header
   s = struct.Struct('IIQQQQQQ')

   def __init__(self, data):
      unpacked_data       = (Elf64_Phdr.s).unpack(data)
      self.unpacked_data  = unpacked_data
      self.p_type         = unpacked_data[0]
      self.p_flags        = unpacked_data[1]
      self.p_offset       = unpacked_data[2]
      self.p_vaddr        = unpacked_data[3]
      self.p_paddr        = unpacked_data[4]
      self.p_filesz       = unpacked_data[5]
      self.p_memsz        = unpacked_data[6]
      self.p_align        = unpacked_data[7]

   def printValues(self):
      print("ATTRIBUTE / VALUE")
      for attr, value in list(self.__dict__.items()):
         print((attr, value))

   def getPackedData(self):
      values = [self.p_type,
                self.p_flags,
                self.p_offset,
                self.p_vaddr,
                self.p_paddr,
                self.p_filesz,
                self.p_memsz,
                self.p_align
               ]

      return (Elf64_Phdr.s).pack(*values)


#----------------------------------------------------------------------------
# CLASS DEFINITIONS END
#----------------------------------------------------------------------------

#----------------------------------------------------------------------------
# BOOT TOOLS BEGIN
#----------------------------------------------------------------------------

#----------------------------------------------------------------------------
# Converts integer to bytes. If length after conversion
# is smaller than given length of byte string, returned value is right-filled
# with 0x00 bytes. Use Little-endian byte order.
#----------------------------------------------------------------------------
def convert_int_to_byte_string(n, l):

    if sys.version_info > (3, 0, 0):
      return int(n).to_bytes(l,byteorder='little')
    else:
     return (''.join([chr((n >> ((l - i - 1) * 8))&0xff) for i in range(l)][::-1]))

#----------------------------------------------------------------------------
# Create default elf header
#----------------------------------------------------------------------------
def create_elf_header( output_file_name,
                       image_dest,
                       image_size,
                       is_elf_64_bit = False):

   if (output_file_name is None):
      raise RuntimeError("Requires a ELF header file")

   # Create a elf header and program header
   # Write the headers to the output file
   elf_fp = open(output_file_name, "wb")

   if (is_elf_64_bit == True):
      # ELf header
      elf_fp.write(ELFINFO_MAG0)
      elf_fp.write(ELFINFO_MAG1)
      elf_fp.write(ELFINFO_MAG2)
      elf_fp.write(ELFINFO_MAG3)
      elf_fp.write(ELFINFO_CLASS_64)
      elf_fp.write(ELFINFO_DATA2LSB)
      elf_fp.write(ELFINFO_VERSION_CURRENT)
      elf_fp.write(pad_bytes(9, ELFINFO_RESERVED))
      elf_fp.write(ELFINFO_EXEC_ETYPE)
      elf_fp.write(ELFINFO_ARM_MACHINETYPE)
      elf_fp.write(ELFINFO_VERSION_EV_CURRENT)
      elf_fp.write(convert_int_to_byte_string(image_dest, 8))
      elf_fp.write(convert_int_to_byte_string(ELF64_HDR_SIZE, 8))
      elf_fp.write(convert_int_to_byte_string(ELFINFO_SHOFF, 8))
      elf_fp.write(pad_bytes(4, ELFINFO_RESERVED))
      elf_fp.write(convert_int_to_byte_string(ELF64_HDR_SIZE, 2))
      elf_fp.write(convert_int_to_byte_string(ELF64_PHDR_SIZE, 2))
      elf_fp.write(ELFINFO_PHNUM)
      elf_fp.write(pad_bytes(6, ELFINFO_RESERVED))

      # Program Header
      elf_fp.write(convert_int_to_byte_string(LOAD_TYPE, 4))
      elf_fp.write(convert_int_to_byte_string(MI_PBT_RWE_SEGMENT, 4))
      elf_fp.write(convert_int_to_byte_string(ELF64_HDR_SIZE+ELF64_PHDR_SIZE, 8))
      elf_fp.write(convert_int_to_byte_string(image_dest, 8))
      elf_fp.write(convert_int_to_byte_string(image_dest, 8))
      elf_fp.write(convert_int_to_byte_string(image_size, 8))
      elf_fp.write(convert_int_to_byte_string(image_size, 8))
      elf_fp.write(convert_int_to_byte_string(ELF_BLOCK_ALIGN, 8))
   else:
      # ELf header
      elf_fp.write(ELFINFO_MAG0)
      elf_fp.write(ELFINFO_MAG1)
      elf_fp.write(ELFINFO_MAG2)
      elf_fp.write(ELFINFO_MAG3)
      elf_fp.write(ELFINFO_CLASS_32)
      elf_fp.write(ELFINFO_DATA2LSB)
      elf_fp.write(ELFINFO_VERSION_CURRENT)
      elf_fp.write(pad_bytes(9, ELFINFO_RESERVED))
      elf_fp.write(ELFINFO_EXEC_ETYPE)
      elf_fp.write(ELFINFO_ARM_MACHINETYPE)
      elf_fp.write(ELFINFO_VERSION_EV_CURRENT)
      elf_fp.write(convert_int_to_byte_string(image_dest, 4))
      elf_fp.write(convert_int_to_byte_string(ELF32_HDR_SIZE, 4))
      elf_fp.write(convert_int_to_byte_string(ELFINFO_SHOFF, 4))
      elf_fp.write(pad_bytes(4, ELFINFO_RESERVED))
      elf_fp.write(convert_int_to_byte_string(ELF32_HDR_SIZE, 2))
      elf_fp.write(convert_int_to_byte_string(ELF32_PHDR_SIZE, 2))
      elf_fp.write(ELFINFO_PHNUM)
      elf_fp.write(pad_bytes(6, ELFINFO_RESERVED))

      # Program Header
      elf_fp.write(convert_int_to_byte_string(LOAD_TYPE, 4))
      elf_fp.write(convert_int_to_byte_string(ELF32_HDR_SIZE+ELF32_PHDR_SIZE, 4))
      elf_fp.write(convert_int_to_byte_string(image_dest, 4))
      elf_fp.write(convert_int_to_byte_string(image_dest, 4))
      elf_fp.write(convert_int_to_byte_string(image_size, 4))
      elf_fp.write(convert_int_to_byte_string(image_size, 4))
      elf_fp.write(convert_int_to_byte_string(MI_PBT_RWE_SEGMENT, 4))
      elf_fp.write(convert_int_to_byte_string(ELF_BLOCK_ALIGN, 4))

   elf_fp.close()
   return 0

#----------------------------------------------------------------------------
# BOOT TOOLS END
#----------------------------------------------------------------------------

#----------------------------------------------------------------------------
# HELPER FUNCTIONS BEGIN
#----------------------------------------------------------------------------

#----------------------------------------------------------------------------
# Concatenates the files listed in 'sources' in order and writes to 'target'
#----------------------------------------------------------------------------
def concat_files (target, sources):
   if type(sources) != list:
      sources = [sources]

   target_file = OPEN(target,'wb')

   for fname in sources:
      file = OPEN(fname,'rb')
      while True:
         bin_data = file.read(65536)
         if not bin_data:
            break
         target_file.write(bin_data)
      file.close()
   target_file.close()

#----------------------------------------------------------------------------
# Preprocess an ELF file and return the ELF Header Object and an
# array of ELF Program Header Objects
#----------------------------------------------------------------------------
def preprocess_elf_file(elf_file_name):

   # Initialize
   elf_fp = OPEN(elf_file_name, 'rb')
   elf_header = Elf_Ehdr_common(elf_fp.read(ELF_HDR_COMMON_SIZE))

   if verify_elf_header(elf_header) == False:
      raise RuntimeError("ELF file failed verification: " + elf_file_name)

   elf_fp.seek(0)

   if elf_header.e_ident[ELFINFO_CLASS_INDEX] == convert_bytes_to_int(ELFINFO_CLASS_64):
      elf_header = Elf64_Ehdr(elf_fp.read(ELF64_HDR_SIZE))
   else:
      elf_header = Elf32_Ehdr(elf_fp.read(ELF32_HDR_SIZE))

   phdr_table = []

   # Verify ELF header information
   if verify_elf_header(elf_header) == False:
      raise RuntimeError("ELF file failed verification: " + elf_file_name)

   # Get program header size
   phdr_size = elf_header.e_phentsize

   # Find the program header offset
   file_offset = elf_header.e_phoff
   elf_fp.seek(file_offset)

   # Read in the program headers
   for i in range(elf_header.e_phnum):
      if elf_header.e_ident[ELFINFO_CLASS_INDEX] == convert_bytes_to_int(ELFINFO_CLASS_64):
         elf_pheader=Elf64_Phdr(elf_fp.read(phdr_size))
         phdr_table.append(elf_pheader)
      else:
         phdr_table.append(Elf32_Phdr(elf_fp.read(phdr_size)))

   elf_fp.close()
   return [elf_header, phdr_table]

#----------------------------------------------------------------------------
# Verify ELF header contents from an input ELF file
#----------------------------------------------------------------------------
def verify_elf_header(elf_header):

   if (elf_header.e_ident[ELFINFO_MAG0_INDEX] != convert_bytes_to_int(ELFINFO_MAG0)) or \
      (elf_header.e_ident[ELFINFO_MAG1_INDEX] != convert_bytes_to_int(ELFINFO_MAG1)) or \
      (elf_header.e_ident[ELFINFO_MAG2_INDEX] != convert_bytes_to_int(ELFINFO_MAG2)) or \
      (elf_header.e_ident[ELFINFO_MAG3_INDEX] != convert_bytes_to_int(ELFINFO_MAG3)) or \
      ((elf_header.e_ident[ELFINFO_CLASS_INDEX] != convert_bytes_to_int(ELFINFO_CLASS_64)) and \
       (elf_header.e_ident[ELFINFO_CLASS_INDEX] != convert_bytes_to_int(ELFINFO_CLASS_32))) or \
      (elf_header.e_ident[ELFINFO_VERSION_INDEX] != convert_bytes_to_int(ELFINFO_VERSION_CURRENT)):

      return False
   else:
      return True


def convert_bytes_to_int(bytes):
    if sys.version_info > (3, 0, 0):
      return int.from_bytes(bytes,byteorder='little')
    else:
      return bytes

def pad_bytes(length,sbyte):
	return (''.rjust(length, chr(sbyte))).encode('utf8')

#----------------------------------------------------------------------------
# pboot_gen_elf
#----------------------------------------------------------------------------
def pboot_gen_elf(env, elf_in_file_name,
                  hash_out_file_name,
                  elf_out_file_name,
                  secure_type='non_secure',
                  hash_seg_max_size=None,
                  last_phys_addr=None,
                  append_xml_hdr=False,
                  is_sha256_algo=True,
                  cert_chain_size_in=CERT_CHAIN_ONEROOT_MAXSIZE,
                  hash_pageseg_as_segment=False):

   global MI_PROG_BOOT_DIGEST_SIZE
   if (is_sha256_algo == True):
      MI_PROG_BOOT_DIGEST_SIZE = 32
   else:
      MI_PROG_BOOT_DIGEST_SIZE = 20

   # Open Files
   elf_in_fp = OPEN(elf_in_file_name, "rb")
   if elf_out_file_name != None:
     elf_out_fp = OPEN(elf_out_file_name, "wb+")
   if hash_out_file_name != None:
     # Generate ELF files with hash segment and hash header table
     hash_out_fp = OPEN(hash_out_file_name, "wb+")

     # Initialize
     [elf_header, phdr_table] = preprocess_elf_file(elf_in_file_name)
     num_phdrs = elf_header.e_phnum
     phdr_total_size = num_phdrs * elf_header.e_phentsize
     phdr_size = elf_header.e_phentsize
     hashtable_size = 0
     hashtable_shift = 0

     if elf_header.e_ident[ELFINFO_CLASS_INDEX] == convert_bytes_to_int(ELFINFO_CLASS_64):
        new_phdr = Elf64_Phdr(b'\0'*ELF64_PHDR_SIZE)
        elf_header_size = ELF64_HDR_SIZE
        is_elf64 = True
     else:
        new_phdr = Elf32_Phdr(b'\0'*ELF32_PHDR_SIZE)
        elf_header_size = ELF32_HDR_SIZE
        is_elf64 = False

     hash = b'\0'*MI_PROG_BOOT_DIGEST_SIZE
     phdr_start = 0
     bytes_to_pad = 0
     hash_seg_end = 0

     # Process program headers if an output elf is specified
     if elf_out_file_name != None:
        # Assert limit on number of program headers in input ELF
        if num_phdrs > MAX_PHDR_COUNT:
           raise RuntimeError("Input ELF has exceeded maximum number of program headers")

        # Create new program header for the ELF Header + Program Headers
        new_phdr.p_type = NULL_TYPE
        new_phdr.p_flags = MI_PBT_ELF_PHDR_SEGMENT

        # If hash table program header is not found, make sure to include it
        elf_header.e_phnum += 2

        # Create an empty hash entry for PHDR_TYPE
        hash_out_fp.write(b'\0'*(MI_PROG_BOOT_DIGEST_SIZE))
        hashtable_size += MI_PROG_BOOT_DIGEST_SIZE

        # Create an empty hash entry for the hash segment itself
        hash_out_fp.write(b'\0'*(MI_PROG_BOOT_DIGEST_SIZE))
        hashtable_size += MI_PROG_BOOT_DIGEST_SIZE

     # Begin hash table generation
     for i in range(num_phdrs):
        curr_phdr = phdr_table[i]

        if (MI_PBT_PAGE_MODE_VALUE(curr_phdr.p_flags) == MI_PBT_PAGED_SEGMENT):
           seg_offset = curr_phdr.p_offset
           seg_size = curr_phdr.p_filesz
           hash_size = 0

           # Check if the vaddr is page aligned
           off = curr_phdr.p_vaddr & (ELF_BLOCK_ALIGN - 1)
           if int(off) != 0:
              seg_size -= (ELF_BLOCK_ALIGN - off)
              seg_offset += (ELF_BLOCK_ALIGN - off)

           # Seg_size should be page aligned
           if (seg_size & (ELF_BLOCK_ALIGN - 1)) > 0:
              raise RuntimeError("seg_size: " + hex(seg_size) + " is not ELF page aligned!")

           off = seg_offset + seg_size

           # Add a single hash table entry for pageable segment
           if hash_pageseg_as_segment:
               elf_in_fp.seek(seg_offset)
               fbuf = elf_in_fp.read(seg_size)

               if MI_PBT_CHECK_FLAG_TYPE(curr_phdr.p_flags) == True:
                   hash = generate_hash(fbuf, is_sha256_algo)
               else:
                   hash = b'\0'*(MI_PROG_BOOT_DIGEST_SIZE)

               # Write hash to file
               hash_out_fp.write(hash)
               hashtable_size += MI_PROG_BOOT_DIGEST_SIZE
           # Add a hash table entry for each block of pageable segment
           else:
               while seg_offset < off:

                  if seg_offset < ELF_BLOCK_ALIGN:
                     hash_size = seg_offset
                  else:
                     hash_size = ELF_BLOCK_ALIGN

                  elf_in_fp.seek(seg_offset)
                  fbuf = elf_in_fp.read(hash_size)

                  if MI_PBT_CHECK_FLAG_TYPE(curr_phdr.p_flags) == True:
                     hash = generate_hash(fbuf, is_sha256_algo)
                  else:
                     hash = b'\0'*(MI_PROG_BOOT_DIGEST_SIZE)

                  # Write hash to file
                  hash_out_fp.write(hash)

                  hashtable_size += MI_PROG_BOOT_DIGEST_SIZE
                  seg_offset += ELF_BLOCK_ALIGN

        # Copy the hash entry for all that are PAGED segments and those that are not the PHDR type. This is for
        # backward tool compatibility where some images are generated using older exe tools.
        elif((MI_PBT_PAGE_MODE_VALUE(curr_phdr.p_flags) == MI_PBT_NON_PAGED_SEGMENT) and (curr_phdr.p_type != PHDR_TYPE)):
           # Read full hash entry into buffer
           elf_in_fp.seek(curr_phdr.p_offset)
           data_len = curr_phdr.p_filesz
           file_buff = elf_in_fp.read(data_len)

           if (MI_PBT_CHECK_FLAG_TYPE(curr_phdr.p_flags) == True) and (data_len > 0):
              hash = generate_hash(file_buff, is_sha256_algo)
           else:
              hash = b'\0'*(MI_PROG_BOOT_DIGEST_SIZE)

           # Write hash to file
           hash_out_fp.write(hash)

           hashtable_size += MI_PROG_BOOT_DIGEST_SIZE
     # End hash table generation

     # Generate the rest of the ELF output file if specified
     if elf_out_file_name != None:

       # Preempt hash table size if necessary
       if secure_type == 'secure':
          hashtable_size += (SHA256_SIGNATURE_SIZE + cert_chain_size_in)

       if append_xml_hdr == True:
          hashtable_size += XML_HEADER_MAXSIZE

       # Initialize the hash table program header
       [hash_Phdr, pad_hash_segment, hash_tbl_end_addr, hash_tbl_offset] = \
            initialize_hash_phdr(elf_in_file_name, hashtable_size, MI_BOOT_IMG_HDR_SIZE, ELF_BLOCK_ALIGN, is_elf64)

       # Check if hash segment max size parameter was passed
       if (hash_seg_max_size != None):
           # Error checking for hash segment size validity
          if hashtable_size > hash_seg_max_size:
             raise RuntimeError("Hash table exceeds maximum hash segment size: " + hex(hash_seg_max_size))
          if (hash_seg_max_size & (ELF_BLOCK_ALIGN-1)) != 0:
             raise RuntimeError("Hash segment size passed is not ELF Block Aligned: " + hex(hash_seg_max_size))

       # Check if hash physical address parameter was passed
       if last_phys_addr != None:
          hash_Phdr.p_vaddr = last_phys_addr
          hash_Phdr.p_paddr = last_phys_addr

       # Check if hash segment max size was passed
       if hash_seg_max_size != None:
          hash_Phdr.p_memsz = hash_seg_max_size

       # Determine the end of the hash segment, make sure it's block aligned
       bytes_to_pad = ELF_BLOCK_ALIGN - pad_hash_segment
       hash_seg_end = hash_tbl_end_addr + bytes_to_pad

       # Check if a shifting is required to accomodate for the hash segment.
       # Get the minimum offset by going through the program headers.
       # Note that the program headers in the input file do not contain
       # the dummy program header for ELF + Program header, and the
       # program header for the hashtable.
       min_offset = hash_seg_end #start with the minimum needed
       for i in range(num_phdrs):
          curr_phdr = phdr_table[i]
          if curr_phdr.p_offset < min_offset and (curr_phdr.p_type != PHDR_TYPE): # discard entry of type PHDR which will have offset=0:
              min_offset = curr_phdr.p_offset

       if min_offset < hash_seg_end:
          hashtable_shift = hash_seg_end - min_offset

       # Move program headers to after ELF header
       phdr_start = elf_header_size

       # We copy over no section headers so assign these values to 0 in ELF Header
       elf_header.e_shnum = 0
       elf_header.e_shstrndx = 0
       elf_header.e_shoff = 0

       # Output remaining ELF segments
       for i in range(num_phdrs):

           # Increment the file offset before writing to the destination file
           curr_phdr = phdr_table[i]

           # We do not copy over program headers of PHDR type, decrement the program
           # header count and continue the loop
           if curr_phdr.p_type == PHDR_TYPE:
              elf_header.e_phnum -= 1
              continue

           src_offset = curr_phdr.p_offset

           # Copy the ELF segment
           file_copy_offset(elf_in_fp, src_offset, elf_out_fp, curr_phdr.p_offset + hashtable_shift, curr_phdr.p_filesz)

       # Output remaining program headers and ELF segments
       elf_header.e_phoff = phdr_start

       # Output new program headers which we have generated
       elf_out_fp.seek(phdr_start)
       new_phdr.p_filesz = elf_header_size + (elf_header.e_phnum * phdr_size)
       elf_out_fp.write(new_phdr.getPackedData())
       elf_out_fp.write(hash_Phdr.getPackedData())
       phdr_start += (2 * phdr_size)

       # Increment the file offset before writing to the destination file
       for i in range(num_phdrs):
           curr_phdr = phdr_table[i]

           if curr_phdr.p_type == PHDR_TYPE:
              continue

           curr_phdr.p_offset += hashtable_shift

           # Copy the program header
           elf_out_fp.seek(phdr_start)
           elf_out_fp.write(curr_phdr.getPackedData())

           # Update phdr_start
           phdr_start += phdr_size

       # Finally, copy the new ELF header to the destination file
       elf_out_fp.seek(0)
       elf_out_fp.write(elf_header.getPackedData())

       # Recalculate hash of ELF + program headers and output to hash output file
       elf_out_fp.seek(0)
       # Read the elf header
       elfhdr_buff = elf_out_fp.read(elf_header_size)
       # Seek to the program header offset listed in elf header.
       elf_out_fp.seek(elf_header.e_phoff)
       # Read the program header and compute hash
       proghdr_buff = elf_out_fp.read(elf_header.e_phnum * phdr_size)

       hash = generate_hash(elfhdr_buff + proghdr_buff, is_sha256_algo)

       # Write hash to file as first hash table entry
       hash_out_fp.seek(0)
       hash_out_fp.write(hash)

     # Close files
     hash_out_fp.close()
   else:
     #generate elf file without hash segment and hash table
     # Initialize
     [elf_header, phdr_table] = preprocess_elf_file(elf_in_file_name)
     if elf_header.e_phnum > 1:
       raise RuntimeError("e_phnum should be not only 1 segment")
     #read Offset of the segment in the input file image.
     seg_offset =  phdr_table[0].p_offset
     #Check if the p_offset is page aligned
     if seg_offset < ELF_BLOCK_ALIGN:
       phdr_table[0].p_offset = ELF_BLOCK_ALIGN
     #write elf header into output file
     elf_out_fp.seek(0)
     elf_out_fp.write(elf_header.getPackedData())

     #write prg header into output file
     elf_out_fp.seek(elf_header.e_ehsize)
     elf_out_fp.write(phdr_table[0].getPackedData())

     #read image segment from input file and write into output file
     elf_in_fp.seek(seg_offset)
     buff = elf_in_fp.read(phdr_table[0].p_filesz)
     elf_out_fp.seek(phdr_table[0].p_offset)
     elf_out_fp.write(buff)

   # Close files
   elf_in_fp.close()
   if elf_out_file_name != None:
      elf_out_fp.close()

   return 0




#----------------------------------------------------------------------------
# Helper functions to open a file and return a valid file object
#----------------------------------------------------------------------------
def OPEN(file_name, mode):
    try:
       fp = open(file_name, mode)
    except IOError:
       raise RuntimeError("The file could not be opened: " + file_name)

    # File open has succeeded with the given mode, return the file object
    return fp
#----------------------------------------------------------------------------
# HELPER FUNCTIONS END
#----------------------------------------------------------------------------

