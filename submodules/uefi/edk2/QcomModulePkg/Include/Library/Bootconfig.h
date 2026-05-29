/* Copyright (c) 2021, The Linux Foundation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * * Redistributions of source code must retain the above copyright
 *  notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following
 * disclaimer in the documentation and/or other materials provided
 *  with the distribution.
 *   * Neither the name of The Linux Foundation nor the names of its
 * contributors may be used to endorse or promote products derived
 * from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef LIBBOOTCONFIG_H_
#define LIBBOOTCONFIG_H_
#define ALIGN(v, a)                     (UINTN)((((v) - 1) | ((a) - 1)) + 1)
/*
 * Add a string of boot config parameters to memory appended by the trailer.
 * This memory needs to be immediately following the end of the ramdisks.
 * The paramters will appended behind the bootconfig paramters passed through
 * bootconfig file from build. The Trailer can be present or not in the
 * bootconfig. But the trailer will be removed if preset after adding new
 * parameters with this function.
 *
 * @param BootconfigStartAddr addr of the start of the bootconfig section in
 *        memory.
 * @param BootconfigSize Size of bootconfig
 * @return V4_BOOTCONFIG_TRAILER if the trailer is copied to the end of the
 *         bootconfig
 *         0 if paramsize is incorrect
 */
UINT64 AddBootconfigParameters ( CHAR8 *Params, UINT32 ParamSize,
                            UINT64 BootconfigStartAddr,
                            UINT32 BootconfigSize );
/*
 * Add the boot config trailer to the end of the boot config parameter section.
 * The new boot config trailer will be written to the end of the entire
 * parameter section at (BootconfigStartAddr + BootconfigSize).
 * The trailer contains a 4 byte size of the parameters, followed by a 4 byte
 * checksum of the parameters, followed by a 12 byte magic string.
 *
 * @param BootconfigStartAddr addr of the start of the bootconfig section in
 *        memory.
 * @param BootconfigSize Size of bootconfig
 * @return V4_BOOTCONFIG_TRAILER if the trailer is copied to the end of the
 *         bootconfig
 *         0 if paramsize is incorrect
 *        -1 if NULL pointers are passed
 */
UINT64 AddBootConfigTrailer ( UINT64 BootconfigStartAddr,
                         UINT32 BootconfigSize );
#endif /*  LIBBOOTCONFIG_H_ */
