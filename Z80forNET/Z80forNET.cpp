// Z80forNET - Z80 snapshot for Interface 1 NET image converter
// Copyright (c) 2024, Tom Dalby
// 
// Z80forNET is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// Z80forNET is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with Z80forNET. If not, see <http://www.gnu.org/licenses/>. 
//  
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#define VERSION_NUM "v1.0"
#define PROGNAME "Z80forNET"
#define B_GAP 200

//
// v1 initial release based on v1.2 of Z80onMDRpi
//
// E01 - input file not Z80 or SNA snapshot
// E02 - cannot open snapshot for read
// E03 - cannot create output file
// E04 - unsupported Z80 snapshot
// E05 - special memory page mode, +3/+2A only so not supported
// E06 - not enough memory
// E07 - input file read error, issue with Z80/SNA snapshot
// E08 - unable to delete temp file
// E09 - unable to compress main memory
// E10 - PC & stack clash & not able to rectify
// E11 - No space on cartridge
//
typedef union {
	uint32_t rrrr; //byte number
	uint8_t r[4]; //split number into 4 8bit bytes in case of overflow
} rrrr;
//
uint16_t dcz80(FILE** fp_in, uint8_t* out, uint16_t size);
uint32_t simplelz(uint8_t* fload, uint8_t* store, uint16_t filesize);
uint16_t decompressf(uint8_t* store, uint16_t filesize, uint16_t size);
void error(uint8_t errorcode);
//
//main
int main(int argc, char* argv[]) {
	// common
	int i, j;
	uint8_t c;
	rrrr len;
	//
	if (argc < 2) error(0);
	if (strcmp(&argv[1][strlen(argv[1]) - 4], ".z80") != 0 && strcmp(&argv[1][strlen(argv[1]) - 4], ".Z80") != 0 &&
		strcmp(&argv[1][strlen(argv[1]) - 4], ".sna") != 0 && strcmp(&argv[1][strlen(argv[1]) - 4], ".SNA") != 0) error(1); // argument isn't .z80/sna or .Z80/SNA
	//create ouput mdr name from input
	char* fz80 = argv[1];
	char fmdr[256]; // limit to 256chars
	for (i = 0; i < strlen(fz80) - 4 && i < 252; i++) fmdr[i] = fz80[i];
	fmdr[i] = '\0';
	strcat(fmdr, ".run");
	//open read/write
	FILE* fp_in, * fp_out;
	if ((fp_in = fopen(fz80, "rb")) == NULL) error(2); // cannot open snapshot for read
	// get filesize
	fseek(fp_in, 0, SEEK_END); // jump to the end of the file to get the length
	int filesize = ftell(fp_in); // get the file size
	rewind(fp_in);
	// z80 or sna?
	int snap = 0;
	if (strcmp(&fz80[strlen(fz80) - 4], ".sna") == 0 || strcmp(&fz80[strlen(fz80) - 4], ".SNA") == 0) snap = 1;
	// basic loader including stage 1
#define mdrbln_brd 16
#define mdrbln_to 51
#define mdrbln_pap 135 // paper/ink
#define mdrbln_fcpy 153 // final copy position
#define mdrbln_cpyf 156 // copy from, normal 0x5b00
#define mdrbln_cpyx 159 // copy times	
#define mdrbln_fffd 195 // last fffd
#define mdrbln_i 210
#define mdrbln_im 214
#define mdrbln_ts 216
#define mdrbln_jp 219 // change if move launcher
#define mdrbln_ay 221 // start of ay array
#define mdrbln_bca 237
#define mdrbln_dea 239
#define mdrbln_hla 241
#define mdrbln_ix 243
#define mdrbln_iy 245
#define mdrbln_afa 247
#define mdrbln_len 250
	uint8_t mdrbln[] = { 0x00,0x00,0x62,0x00,0xfd,0x30,0x0e,0x00,											//(0)
								0x00,0x4f,0x61,0x00,0x3a,0xe7,0xb0,0x22,0x30,0x22,									//(8) clear 24911 0x4f,0x61
								0x3a,0xf9,0xc0,0x30,0x0e,0x00,0x00,0x70,0x5d,0x00,0x3a,0xf1,0x64,0x3d,				//(18) randomize usr 23920
								0xbe,0x30,0x0e,0x00,0x00,0xd6,0x5c,0x00,0x3a,										//(32) let d=peek 23766
								0xeb,0x69,0x3d,0xb0,0x22,0x30,0x22,0xcc,0xb0,0x22,0x35,0x22,0x3a,0xef,0x2a,0x22,	//(41)
								0x6e,0x22,0x3b,0x64,0x3b,0xc1,0x69,0xaf,0x3a,0xf9,0xc0,0x30,0x0e,0x00,0x00,0xc9,	//(57) *"n";d; randomize usr 32179->32201
								0x7d,0x00,0x3a,0xf3,0x69,0x3a,0xef,0x2a,0x22,0x6e,0x22,0x3b,0x64,0x3b,0x22,0x4d,	//(73) *"n";d;"M"
								0x22,0xaf,0x3a,																		//(89)
								0xf9,0xc0,0x30,0x0e,0x00,0x00,0x9c,0x5d,0x00,0x0d,									//(92) randomize usr 23964
		// usr 0 code
		0x27,0x0f,0x90,0x00,0xea,															//(102) line9999
		//
		0xf3,0x2a,0x3d,0x5c,0x23,0x36,0x13,0x2b,0x36,0x03,0x2b,0x36,0x1b,0x2b,0x36,0x76,	//(107) usr 0
		0x2b,0x36,0x00,0x2b,0x36,0x51,0xf9,0xfd,0xcb,0x01,0xa6,0x3e,0x00,0x32,0x8d,0x5c,	//(123)
		0xcd,0xaf,0x0d,0x3e,0x10,0x01,0xfd,0x7f,0xed,0x79,0xfb,0xc9,						//(139)
		// stage 1
		0xf3,0x21,0x39,0x30,0x11,0x00,0x5b,0x01,0x21,0x00,0xed,								//(151) 
		0xb0,0x31,0xe2,0x5d,0xd9,0x01,0xfd,0xff,0xaf,0xe1,0xed,0x79,0x3c,0x06,0xbf,0xed,	//(162)
		0x69,0x06,0xff,0xed,0x79,0x3c,0x06,0xbf,0xed,0x61,0xfe,0x10,0x06,0xff,0x20,0xe9,	//(178)
		0x3e,0x00,0xed,0x79,0xc1,0xd1,0xe1,0xd9,0xdd,0xe1,0xfd,0xe1,0x08,0xf1,0x08,0x3e,	//(194)
		0x00,0xed,0x47,0xed,0x5e,0x31,0x1f,0x5b,0xc3,0x04,0x5b,0x00,0x00,0x00,0x00,0x00,	//(210)
		0x00,0x00,0xff,0x00,0x00,0x00,0x00,0x00,0x00,0xbf,0x00,0x00,0x00,0x00,0x00,0x00,	//(226)
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x0d };											//(242)								

	// stage 2 - printer buffer
#define noc_launchprt_jp 9 // where to jump to
#define noc_launchprt_len 31
	uint8_t noc_launchprt[] = { 0x3c,0x4f,0xed,0xb0,0x7e,0x23,0xfe,0x80,0xca,0x21,0x5b,0x38,0xf3,0xd6,0x7e,0x4e,
										0x23,0xe5,0x62,0x6b,0xed,0x42,0x2b,0x4f,0xed,0xb0,0xe1,0x18,0xe7,0x00,0x00 };
	// stage 3 - gap part
	int noc_launchigp_pos = 0; // memory position for this routine
#define noc_launchigp_bdata 1 // bdata start, pos+16
#define noc_launchigp_lcs 4 // last copy size=delta=3
#define noc_launchigp_de 14
#define noc_launchigp_clr 18 // amount to clear, max 256 (coded as 0), min 59
#define noc_launchigp_chr 17 // char to clear
#define noc_launchigp_rd 20 // stack rdata = stack-2
#define noc_launchigp_jp 23 // jump into = stack-28
#define noc_launchigp_begin 25 // beginning of bdata
#define noc_launchigp_len 59 // 25 + 3 + 31 = 59bytes for delta=3, nax delta=256-31-25=200
	uint8_t noc_launchigp[] = { 0x21,0x4f,0x5b,0x0e,0x03,0xed,0xb0,0x16,0x5b,0x0e,0x1f,0xed,0xb0,	//(0)
										0x11,0x00,0x00,0x01,0x00,0x3d,0x31,0x64,0x5b,0xc3,0x4f,0x5b };		//(13)
	// stage 4 - stack part
	int noc_launchstk_pos = 0; // memory position stack - noc_launchstk_len
#define noc_launchstk_out 8
#define noc_launchstk_bc 12
#define noc_launchstk_hl 15
#define noc_launchstk_r 18 // r
#define noc_launchstk_ei 22
#define noc_launchstk_jp 24
#define noc_launchstk_af 26
#define noc_launchstk_len 28
	uint8_t noc_launchstk[] = { 0x2b,0x71,0x10,0xfc,0x01,0xfd,0x7f,0x3e,0x30,0xed,0x79,0x01,0x00,0x00,0x21,0x00,	//(0)
										0x00,0x3e,0x02,0xed,0x4f,0xf1,0xf3,0xc3,0xb7,0xd9,0x00,0x00 };						//(16)
	//compressed screen loader @32201
#define scrload_len 36
	const uint8_t scrload[] = { 0x21,0xed,0x7d,0x11,0x00,0x40,0x43,0x18,0x04,0x3c,0x4f,0xed,0xb0,0x7e,0x23,0xfe,
								0x80,0xc8,0x38,0xf5,0xd6,0x7e,0x4e,0x23,0xe5,0x62,0x6b,0xed,0x42,0x2b,0x4f,0xed,
								0xb0,0xe1,0x18,0xe9 };

	//unpacker for 128k pages @32201
#define unpack_len 55
	const uint8_t unpack[] = { 0xf3,0x21,0xff,0x7d,0x7e,0x01,0xfd,0x7f,0xed,0x79,0x23,0x11,0x00,0xc0,0x43,0xcd,
								0xe8,0x7d,0x3e,0x10,0x01,0xfd,0x7f,0xed,0x79,0xfb,0xc9,0x3c,0x4f,0xed,0xb0,0x7e,
								0x23,0xfe,0x80,0xc8,0x38,0xf5,0xd6,0x7e,0x4e,0x23,0xe5,0x62,0x6b,0xed,0x42,0x2b,
								0x4f,0xed,0xb0,0xe1,0x18,0xe9,0x11 };
	//
	int otek = 0, stackpos = 0;
	uint8_t compressed = 0;
	rrrr addlen;
	addlen.rrrr = 0; // z80 only, 0 indicates v1, 23 for v2 otherwise v3
	//read is sna, compressed=0, addlen.rrrr=0, otek=0
	if (snap) {
		if (filesize < 49179) error(7);
		if (filesize >= 131103) otek = 1; // 128k snapshot
		//	$00  I	Interrupt register
		mdrbln[mdrbln_i] = fgetc(fp_in);
		//	$01  HL'
		mdrbln[mdrbln_hla] = fgetc(fp_in);
		mdrbln[mdrbln_hla + 1] = fgetc(fp_in);
		//	$03  DE'
		mdrbln[mdrbln_dea] = fgetc(fp_in);
		mdrbln[mdrbln_dea + 1] = fgetc(fp_in);
		// check this is a SNA snapshot
		if (mdrbln[mdrbln_i] == 'M' && mdrbln[mdrbln_hla] == 'V' &&
			mdrbln[mdrbln_hla + 1] == ' ' && mdrbln[mdrbln_dea] == '-') error(7);
		if (mdrbln[mdrbln_i] == 'Z' && mdrbln[mdrbln_hla] == 'X' &&
			mdrbln[mdrbln_hla + 1] == '8' && mdrbln[mdrbln_dea] == '2') error(7);
		//	$05  BC'
		mdrbln[mdrbln_bca] = fgetc(fp_in);
		mdrbln[mdrbln_bca + 1] = fgetc(fp_in);
		//	$07  F'
		mdrbln[mdrbln_afa] = fgetc(fp_in);
		//	$08  A'
		mdrbln[mdrbln_afa + 1] = fgetc(fp_in);
		//	$09  HL	
		noc_launchstk[noc_launchstk_hl] = fgetc(fp_in);
		noc_launchstk[noc_launchstk_hl + 1] = fgetc(fp_in);
		//	$0B  DE
		noc_launchigp[noc_launchigp_de] = fgetc(fp_in);
		noc_launchigp[noc_launchigp_de + 1] = fgetc(fp_in);
		//	$0D  BC
		noc_launchstk[noc_launchstk_bc] = fgetc(fp_in);
		noc_launchstk[noc_launchstk_bc + 1] = fgetc(fp_in);
		//	$0F  IY
		mdrbln[mdrbln_iy] = fgetc(fp_in);
		mdrbln[mdrbln_iy + 1] = fgetc(fp_in);
		//	$11  IX
		mdrbln[mdrbln_ix] = fgetc(fp_in);
		mdrbln[mdrbln_ix + 1] = fgetc(fp_in);
		//	$13  0 for DI otherwise EI
		c = fgetc(fp_in);
		if (c == 0) noc_launchstk[noc_launchstk_ei] = 0xf3;	//di
		else noc_launchstk[noc_launchstk_ei] = 0xfb;	//ei
		//	$14  R
		noc_launchstk[noc_launchstk_r] = fgetc(fp_in);
		//	$15  F
		noc_launchstk[noc_launchstk_af] = fgetc(fp_in);
		//	$16  A
		noc_launchstk[noc_launchstk_af + 1] = fgetc(fp_in);
		//	$17  SP
		stackpos = fgetc(fp_in);
		stackpos = stackpos + fgetc(fp_in) * 256;
		if (!otek)stackpos += 2;
		if (stackpos == 0) stackpos = 65536;
		noc_launchstk_pos = stackpos - noc_launchstk_len; // pos of stack code
		len.rrrr = noc_launchstk_pos + noc_launchstk_af;
		noc_launchigp[noc_launchigp_rd] = len.r[0];
		noc_launchigp[noc_launchigp_rd + 1] = len.r[1]; // start of stack within stack
		// $19  Interrupt mode IM(0, 1 or 2)
		c = fgetc(fp_in) & 3;
		if (c == 0) mdrbln[mdrbln_im] = 0x46; //im 0
		else if (c == 1) mdrbln[mdrbln_im] = 0x56; //im 1
		else mdrbln[mdrbln_im] = 0x5e; //im 2
		//	$1A  Border colour
		c = fgetc(fp_in) & 7;
		mdrbln[mdrbln_brd] = c + 0x30;
		mdrbln[mdrbln_pap] = (c << 3) + c;
	}
	// read z80
	else {
		//read in z80 starting with header
		//	0       1       A register
		noc_launchstk[noc_launchstk_af + 1] = fgetc(fp_in);
		//	1       1       F register
		noc_launchstk[noc_launchstk_af] = fgetc(fp_in);
		//	2       2       BC register pair(LSB, i.e.C, first)
		noc_launchstk[noc_launchstk_bc] = fgetc(fp_in);
		noc_launchstk[noc_launchstk_bc + 1] = fgetc(fp_in);
		//	4       2       HL register pair
		noc_launchstk[noc_launchstk_hl] = fgetc(fp_in);
		noc_launchstk[noc_launchstk_hl + 1] = fgetc(fp_in);
		//	6       2       Program counter (if zero then version 2 or 3 snapshot)
		noc_launchstk[noc_launchstk_jp] = fgetc(fp_in);
		noc_launchstk[noc_launchstk_jp + 1] = fgetc(fp_in);
		//	8       2       Stack pointer
		stackpos = fgetc(fp_in);
		stackpos = stackpos + fgetc(fp_in) * 256;
		if (stackpos == 0) stackpos = 65536;
		noc_launchstk_pos = stackpos - noc_launchstk_len; // pos of stack code
		len.rrrr = noc_launchstk_pos + noc_launchstk_af;
		noc_launchigp[noc_launchigp_rd] = len.r[0];
		noc_launchigp[noc_launchigp_rd + 1] = len.r[1]; // start of stack within stack
		//	10      1       Interrupt register
		mdrbln[mdrbln_i] = fgetc(fp_in);
		//	11      1       Refresh register (Bit 7 is not significant!)
		c = fgetc(fp_in);
		noc_launchstk[noc_launchstk_r] = c - 3; // 3 for 4 stage launcher
		//	12      1       Bit 0: Bit 7 of r register; Bit 1-3: Border colour; Bit 4=1: SamROM; Bit 5=1:v1 Compressed; Bit 6-7: N/A
		c = fgetc(fp_in);
		compressed = (c & 32) >> 5;	// 1 compressed, 0 not
		if (c & 1 || c > 127) {
			noc_launchstk[noc_launchstk_r] = noc_launchstk[noc_launchstk_r] | 128;	// r high bit set
		}
		else {
			noc_launchstk[noc_launchstk_r] = noc_launchstk[noc_launchstk_r] & 127;	//r high bit reset
		}
		mdrbln[mdrbln_brd] = ((c & 14) >> 1) + 0x30; //border
		mdrbln[mdrbln_pap] = (((c & 14) >> 1) << 3) + ((c & 14) >> 1); //paper/ink
		//	13      2       DE register pair
		noc_launchigp[noc_launchigp_de] = fgetc(fp_in);
		noc_launchigp[noc_launchigp_de + 1] = fgetc(fp_in);
		//	15      2       BC' register pair
		mdrbln[mdrbln_bca] = fgetc(fp_in);
		mdrbln[mdrbln_bca + 1] = fgetc(fp_in);
		//	17      2       DE' register pair
		mdrbln[mdrbln_dea] = fgetc(fp_in);
		mdrbln[mdrbln_dea + 1] = fgetc(fp_in);
		//	19      2       HL' register pair
		mdrbln[mdrbln_hla] = fgetc(fp_in);
		mdrbln[mdrbln_hla + 1] = fgetc(fp_in);
		//	21      1       A' register
		mdrbln[mdrbln_afa + 1] = fgetc(fp_in);
		//	22      1       F' register
		mdrbln[mdrbln_afa] = fgetc(fp_in);
		//	23      2       IY register (Again LSB first)
		mdrbln[mdrbln_iy] = fgetc(fp_in);
		mdrbln[mdrbln_iy + 1] = fgetc(fp_in);
		//	25      2       IX register
		mdrbln[mdrbln_ix] = fgetc(fp_in);
		mdrbln[mdrbln_ix + 1] = fgetc(fp_in);
		//	27      1       Interrupt flipflop, 0 = DI, otherwise EI
		c = fgetc(fp_in);
		if (c == 0) noc_launchstk[noc_launchstk_ei] = 0xf3;	//di
		else noc_launchstk[noc_launchstk_ei] = 0xfb;	//ei
		//	28      1       IFF2 [IGNORED]
		c = fgetc(fp_in);
		//	29      1       Bit 0-1: IM(0, 1 or 2); Bit 2-7: N/A
		c = fgetc(fp_in) & 3;
		if (c == 0) mdrbln[mdrbln_im] = 0x46; //im 0
		else if (c == 1) mdrbln[mdrbln_im] = 0x56; //im 1
		else mdrbln[mdrbln_im] = 0x5e; //im 2
		// version 2 & 3 only
		if (noc_launchstk[noc_launchstk_jp] == 0 && noc_launchstk[noc_launchstk_jp + 1] == 0) {
			//  30      2       Length of additional header block
			addlen.r[0] = fgetc(fp_in);
			addlen.r[1] = fgetc(fp_in);
			//  32      2       Program counter
			noc_launchstk[noc_launchstk_jp] = fgetc(fp_in);
			noc_launchstk[noc_launchstk_jp + 1] = fgetc(fp_in);
			//	34      1       Hardware mode standard 0-6 (2 is SamRAM), 7 +3, 8 +3 & 10 not supported, 11 Didatik, 12 +2, 13 +2A
			c = fgetc(fp_in);
			if (c == 2 || c == 10 || c == 11 || c > 13) error(4);
			if (addlen.rrrr == 23 && c > 2) otek = 1; // v2 & c>2 then 128k, if v3 then c>3 is 128k
			else if (c > 3) otek = 1;
			//	35      1       If in 128 mode, contains last OUT to 0x7ffd
			c = fgetc(fp_in);
			if (otek) noc_launchstk[noc_launchstk_out] = c;
			//	36      1       Contains 0xff if Interface I rom paged [SKIPPED]
			//	37      1       Hardware Modify Byte [SKIPPED]
			fseek(fp_in, 2, SEEK_CUR);
			//	38      1       Last OUT to port 0xfffd (soundchip register number)
			//	39      16      Contents of the sound chip registers
			mdrbln[mdrbln_fffd] = fgetc(fp_in);	// last out to $fffd (38)
			for (i = 0; i < 16; i++) mdrbln[mdrbln_ay + i] = fgetc(fp_in); // ay registers (39-54) 
			// following is only in v3 snapshots
			//	55      2       Low T state counter [SKIPPED]
			//	57      1       Hi T state counter [SKIPPED]
			//	58      1       Flag byte used by Spectator(QL spec.emulator) [SKIPPED]
			//	59      1       0xff if MGT Rom paged [SKIPPED]
			//	60      1       0xff if Multiface Rom paged.Should always be 0. [SKIPPED]
			//	61      1       0xff if 0 - 8191 is ROM, 0 if RAM [SKIPPED]
			//	62      1       0xff if 8192 - 16383 is ROM, 0 if RAM [SKIPPED]
			//	63      10      5 x keyboard mappings for user defined joystick [SKIPPED]
			//	73      10      5 x ASCII word : keys corresponding to mappings above [SKIPPED]
			//	83      1       MGT type : 0 = Disciple + Epson, 1 = Disciple + HP, 16 = Plus D [SKIPPED]
			//	84      1       Disciple inhibit button status : 0 = out, 0ff = in [SKIPPED]
			//	85      1       Disciple inhibit flag : 0 = rom pageable, 0ff = not [SKIPPED]
			if (addlen.rrrr > 23) fseek(fp_in, 31, SEEK_CUR);
			// only if version 3 & 55 additional length
			//	86      1       Last OUT to port 0x1ffd, ignored for NET as only applicable on +3/+2A machines [SKIPPED]
			if (addlen.rrrr == 55) 	if ((fgetc(fp_in) & 1) == 1) error(5); //special page mode so exit as not compatible with earlier 128k machines
		}
	}
	// space for decompression of z80
	// 8 * 16384 = 131072bytes
	//     0- 49152 - Pages 5,2 & 0 (main memory)
	// *128k only - 49152-65536: Page 1; 65536-81920: Page 3; 81920-98304: Page 4; 98304-114688: Page 6; 114688-131072: Page 7
	//uint8_t* main;
	int fullsize = 49152;
	if (otek) fullsize = 131072;
	FILE* fptmp;
	if ((fptmp = fopen(".tmp", "w+b")) == NULL) error(3);
	for (i = 0; i < fullsize; i++) fputc(0x00, fptmp);
	rewind(fptmp);
	uint8_t* main48k;
	if ((main48k = (uint8_t*)malloc(49152 * sizeof(uint8_t))) == NULL) error(6); // cannot create space for copy of main memory
	// memory usage=49152
	// unpack snapshot
	len.rrrr = 0;
	//set-up bank locations
	int bank[11], bankend;
	for (i = 0; i < 11; i++) bank[i] = 99; //default
	if (otek) {
		bank[3] = 32768; //page 0
		bank[4] = 49152; //page 1
		bank[5] = 16384; //page 2
		bank[6] = 65536; //page 3
		bank[7] = 81920; //page 4
		bank[8] = 0; //page 5
		bank[9] = 98304; //page 6
		bank[10] = 114688; //page 7
		bankend = 8;
	}
	else {
		bank[4] = 16384; //page 2
		bank[5] = 32768; //page 0
		bank[8] = 0; //page 5
		bankend = 3;
	}
	if (addlen.rrrr == 0) { // SNA or version 1 z80 snapshot & 48k only
		if (!compressed) {
			if (fread(main48k, sizeof(uint8_t), 49152, fp_in) != 49152) error(7);
			if (fwrite(main48k, sizeof(uint8_t), 49152, fptmp) != 49152) error(7);
			// sort out pc for SNA
			if (snap) {
				fseek(fptmp, stackpos - 16384 - 2, SEEK_SET);
				noc_launchstk[noc_launchstk_jp] = fgetc(fptmp);
				noc_launchstk[noc_launchstk_jp + 1] = fgetc(fptmp);
			}
		}
		else {
			if (dcz80(&fp_in, main48k, 49152) != 49152) error(7);
			if (fwrite(main48k, sizeof(uint8_t), 49152, fptmp) != 49152) error(7);
		}
		if (otek) {
			// PC
			noc_launchstk[noc_launchstk_jp] = fgetc(fp_in);
			noc_launchstk[noc_launchstk_jp + 1] = fgetc(fp_in);
			// last out to 0x7ffd
			noc_launchstk[noc_launchstk_out] = fgetc(fp_in);
			// TD-DOS
			if (fgetc(fp_in) != 0) error(7);
			uint32_t pagelayout[7];
			for (i = 0; i < 7; i++) pagelayout[i] = 99;
			pagelayout[0] = noc_launchstk[noc_launchstk_out] & 7;
			//
			if (pagelayout[0] == 0) {
				pagelayout[0] = 32768;
				pagelayout[1] = 49152;
				pagelayout[2] = 65536;
				pagelayout[3] = 81920;
				pagelayout[4] = 98304;
				pagelayout[5] = 114688;
			}
			else if (pagelayout[0] == 1) {
				pagelayout[0] = 49152;
				pagelayout[1] = 32768;
				pagelayout[2] = 65536;
				pagelayout[3] = 81920;
				pagelayout[4] = 98304;
				pagelayout[5] = 114688;
			}
			else if (pagelayout[0] == 2) {
				pagelayout[0] = 16384;
				pagelayout[1] = 32768;
				pagelayout[2] = 49152;
				pagelayout[3] = 65536;
				pagelayout[4] = 81920;
				pagelayout[5] = 98304;
				pagelayout[6] = 114688;
			}
			else if (pagelayout[0] == 3) {
				pagelayout[0] = 65536;
				pagelayout[1] = 32768;
				pagelayout[2] = 49152;
				pagelayout[3] = 81920;
				pagelayout[4] = 98304;
				pagelayout[5] = 114688;
			}
			else if (pagelayout[0] == 4) {
				pagelayout[0] = 81920;
				pagelayout[1] = 32768;
				pagelayout[2] = 49152;
				pagelayout[3] = 65536;
				pagelayout[4] = 98304;
				pagelayout[5] = 114688;
			}
			else if (pagelayout[0] == 5) {
				pagelayout[0] = 0;
				pagelayout[1] = 32768;
				pagelayout[2] = 49152;
				pagelayout[3] = 65536;
				pagelayout[4] = 81920;
				pagelayout[5] = 98304;
				pagelayout[6] = 114688;
			}
			else if (pagelayout[0] == 6) {
				pagelayout[0] = 98304;
				pagelayout[1] = 32768;
				pagelayout[2] = 49152;
				pagelayout[3] = 65536;
				pagelayout[4] = 81920;
				pagelayout[5] = 114688;
			}
			else {
				pagelayout[0] = 114688;
				pagelayout[1] = 32768;
				pagelayout[2] = 49152;
				pagelayout[3] = 65536;
				pagelayout[4] = 81920;
				pagelayout[5] = 98304;
			}
			if (pagelayout[0] != 32768) {
				fseek(fptmp, pagelayout[0], SEEK_SET);
				if (fwrite(&main48k[32768], sizeof(uint8_t), 16384, fptmp) != 16384) error(7);
			}
			for (i = 1; i < 7; i++) {
				if (pagelayout[i] != 99) {
					fseek(fptmp, pagelayout[i], SEEK_SET);
					if (fread(main48k, sizeof(uint8_t), 16384, fp_in) != 16384) error(7);
					if (fwrite(main48k, sizeof(uint8_t), 16384, fptmp) != 16384) error(7);
				}
			}
		}
	}
	// version 2 & 3
	else {
		//		Byte    Length  Description
		//		-------------------------- -
		//		0       2       Length of compressed data(without this 3 - byte header)
		//						If length = 0xffff, data is 16384 bytes longand not compressed
		//		2       1       Page number of block
		// for 48k snapshots the order is:
		//		0 48k ROM, 1, IF1/PLUSD/DISCIPLE ROM, 4 page 2, 5 page 0, 8 page 5, 11 MF ROM 
		//		only 4, 5 & 8 are valid for this usage, all others are just ignored
		// for 128k snapshots the order is:
		//		0 ROM, 1 ROM, 3 Page 0....10 page 7, 11 MF ROM.
		// all pages are saved and there is no end marker
		do {
			len.r[0] = fgetc(fp_in);
			len.r[1] = fgetc(fp_in);
			c = fgetc(fp_in);
			if (bank[c] != 99) {
				fseek(fptmp, bank[c], SEEK_SET);
				if (len.rrrr == 65535) {
					if (fread(main48k, sizeof(uint8_t), 16384, fp_in) != 16384) error(7);
					if (fwrite(main48k, sizeof(uint8_t), 16384, fptmp) != 16384) error(7);
				}
				else {
					if (dcz80(&fp_in, main48k, 16384) != 16384) error(7);
					if (fwrite(main48k, sizeof(uint8_t), 16384, fptmp) != 16384) error(7);
				}
			}
			bankend--;
		} while (bankend);
	}
	fclose(fp_in);
	//
	if (stackpos < 23296) { // stack in screen?
		i = noc_launchstk[noc_launchstk_jp + 1] * 256 + noc_launchstk[noc_launchstk_jp] - 16384;
		fseek(fptmp, i, SEEK_SET);
		if (fgetc(fptmp) == 0x31) { // ld sp,
			// set-up stack
			c = fgetc(fptmp);
			stackpos = fgetc(fptmp) * 256 + c;
			if (stackpos == 0) stackpos = 65536;
			noc_launchstk_pos = stackpos - noc_launchstk_len; // pos of stack code
			len.rrrr = noc_launchstk_pos + noc_launchstk_af;
			noc_launchigp[noc_launchigp_rd] = len.r[0];
			noc_launchigp[noc_launchigp_rd + 1] = len.r[1]; // start of stack within stack
		}
	}
	else if ((noc_launchstk[noc_launchstk_out] & 7) > 0 && stackpos > 49152 && otek) error(7); // stack in paged memory won't work
	// compress main
	rrrr start;
	//rrrr param;
	rrrr cmsize;
	uint8_t* comp;
	// main
	int delta = 3;
	int vgap = 0;
	int vgaps, vgapb;
	int maxgap, maxpos, maxchr, stshift = 0;
	uint16_t dgap = 0;
	int startpos = 6912 + noc_launchprt_len; // 0x5b19 onwards
	int mainsize = 49152 - startpos;
	int maxsize = 40624; // 0x6110 onwards *0x614f 40625bytes
	if ((comp = (uint8_t*)malloc((mainsize + 319 + 287) * sizeof(uint8_t))) == NULL) error(6);
	// memory usage=49152+42846=91998Bytes
	do {
		rewind(fptmp);
		if (fread(main48k, sizeof(uint8_t), 49152, fptmp) != 49152) error(7);
		noc_launchigp_pos = 0;
		// find maximum gap
		maxgap = maxpos = maxchr = 0;
		for (vgap = 0x00; vgap <= 0xff; vgap++) { // cycle through all bytes
			for (i = 0, j = 0; i < mainsize; i++) { // also include rest of printer buffer
				if (main48k[i + 6912 + noc_launchprt_len] == vgap) {
					j++;
					if (j > maxgap && ((i + 6912 + noc_launchprt_len - j) > stackpos - 16384 || // start of gap > stack then ok
						i + 6912 + noc_launchprt_len < stackpos - 16384 - noc_launchstk_len)) { // end of gap < stack - 32 then ok
						maxgap = j;
						maxpos = i + 1;
						maxchr = vgap;
					}
				}
				else {
					j = 0;
				}
			}
		}
		if (maxgap > (noc_launchigp_len + delta - 3)) {
			noc_launchigp_pos = maxpos + 6912 + noc_launchprt_len - maxgap; // start of in gap 
		}
		// cannot find large enough gap so use screen attr
		else {
			noc_launchigp_pos = 6912 - (noc_launchigp_len + delta - 3);
			vgaps = 0x00;
			vgapb = 0;
			for (maxchr = 0x00; maxchr <= 0xff; maxchr++) {	//find most common attr
				for (i = noc_launchigp_pos, j = 0; i < 6912; i++) {
					if (main48k[i] == maxchr) j++;
				}
				if (j >= vgapb) {
					vgapb = j;
					vgaps = maxchr;
				}
			}
			maxchr = vgaps;
		}
		// is pc in the way?
		if (noc_launchstk_pos <= (noc_launchstk[noc_launchstk_jp + 1] * 256 + noc_launchstk[noc_launchstk_jp]) &&
			noc_launchstk_pos + noc_launchstk_len > (noc_launchstk[noc_launchstk_jp + 1] * 256 + noc_launchstk[noc_launchstk_jp])) {
			stshift = stackpos - (noc_launchstk[noc_launchstk_jp + 1] * 256 + noc_launchstk[noc_launchstk_jp]); // stack - pc
			if (stshift <= 2) error(10);
			stshift = noc_launchstk_af; // shift equivalent of 32bytes below where is was (4bytes still remain under the stack)
		}
		start.rrrr = noc_launchigp_pos + 16384;
		noc_launchprt[noc_launchprt_jp] = start.r[0];
		noc_launchprt[noc_launchprt_jp + 1] = start.r[1]; // jump into gap
		start.rrrr = noc_launchigp_pos + noc_launchigp_begin + 16384;
		noc_launchigp[noc_launchigp_bdata] = start.r[0];
		noc_launchigp[noc_launchigp_bdata + 1] = start.r[1]; // bdata start
		noc_launchigp[noc_launchigp_lcs] = delta;
		if (noc_launchigp_len + delta - 3 == 256) {
			noc_launchigp[noc_launchigp_clr] = 0x00;
		}
		else {
			noc_launchigp[noc_launchigp_clr] = noc_launchigp_len + delta - 3; // size of ingap clear
		}
		noc_launchigp[noc_launchigp_chr] = maxchr; // set the erase char in stack code
		start.rrrr = noc_launchstk_pos - stshift;
		noc_launchigp[noc_launchigp_jp] = start.r[0];
		noc_launchigp[noc_launchigp_jp + 1] = start.r[1]; // jump to stack code - adjust - shift
		// copy stack routine under stack, split version if shift
		if (stshift) {
			for (i = 0; i < noc_launchstk_len - 2; i++) main48k[noc_launchstk_pos - 16384 + i - stshift] = noc_launchstk[i];
			//final 2bytes just below new code
			for (i = 0; i < 2; i++) main48k[stackpos - 16384 + i - 2] = noc_launchstk[noc_launchstk_len - 2 + i];
		}
		else {
			for (i = 0; i < noc_launchstk_len; i++) main48k[noc_launchstk_pos - 16384 + i] = noc_launchstk[i]; // standard copy
		}
		// if ingap not in screen attr, this is done after so as to not effect the screen compression
		if (noc_launchigp_pos >= 6912) {
			// copy prtbuf to code
			for (i = 0; i < noc_launchprt_len; i++) main48k[noc_launchigp_pos + noc_launchigp_begin + delta + i] = main48k[6912 + i];
			// copy delta to code
			for (i = 0; i < delta; i++) main48k[noc_launchigp_pos + noc_launchigp_begin + i] = main48k[49152 - delta + i];
			// copy in compression routine into main
			for (i = 0; i < noc_launchigp_begin; i++) main48k[noc_launchigp_pos + i] = noc_launchigp[i];
		}
		cmsize.rrrr = simplelz(&main48k[startpos], &comp[287], mainsize - delta); // upto the full size - delta
		dgap = decompressf(&comp[287], cmsize.rrrr, mainsize);
		delta += dgap;
		if (delta > B_GAP) error(9);
	} while (dgap > 0);
	// sort out adder - 31, max size 256+31=287
	int adder = 0;
	adder = noc_launchprt_len; // just add prtbuf launcher, 31bytes
	if (noc_launchigp_pos < 6912) adder += noc_launchprt_len + delta + noc_launchigp_begin; // if ingap in screen, 31+31+200+25=256
	maxsize -= delta;
	cmsize.rrrr += adder;
	if (delta > B_GAP || cmsize.rrrr > maxsize) error(9); // too big to fit in Spectrum memory
	// BASIC
	uint8_t mdrfname[] = "run       ";
	// sort out compression start
	len.rrrr = 23296 + noc_launchprt_len - adder; // compress start
	mdrbln[mdrbln_cpyf] = len.r[0];
	mdrbln[mdrbln_cpyf + 1] = len.r[1];
	len.rrrr = adder;
	mdrbln[mdrbln_cpyx] = len.r[0];
	mdrbln[mdrbln_cpyx + 1] = len.r[1];
	// sort out compression start
	len.rrrr = 65536 - cmsize.rrrr; // compress start
	mdrbln[mdrbln_fcpy] = len.r[0];
	mdrbln[mdrbln_fcpy + 1] = len.r[1];
	start.rrrr = 23813;
	//param.rrrr = 0;
	if (!otek) mdrbln[mdrbln_to] = 0x30; // turn off 128k pagin for max 48k compatibility
	// microdrive settings
	//uint8_t sector = 0xfe; // max size 254 sectors
	//uint8_t* u_sector;
	//if ((u_sector = (uint8_t*)malloc(254 * sizeof(uint8_t))) == NULL) error(6);
	//memset(u_sector, 0x00, 254); // clear used sector array
	//uint8_t mdrname[] = "          ";
	//i = 0;
	//int mp = 0;
	//do {
	//	if ((fz80[i] >= 48 && fz80[i] < 58) || (fz80[i] >= 65 && fz80[i] < 91) || (fz80[i] >= 97 && fz80[i] < 123)) mdrname[mp++] = fz80[i];
	//	i++;
	//} while (i < strlen(fz80) - 4 && mp < 10);
	// create a blank cartridge, 137923bytes 254 secotrs * 543 + 1
	// 137923/4096sectors -> 34 or 139264bytes
	// 2097152 -> 512 sectors -> 8 cartridges is 272 sectors
	//rrrr chksum;
	if ((fp_out = fopen(fmdr, "wb")) == NULL) error(3); // cannot open run file for write
	//do {
	//	// header
	//	chksum.rrrr = sector + 1;
	//	fputc(0x01, fp_out);
	//	fputc(sector--, fp_out);
	//	fputc(0x00, fp_out);
	//	fputc(0x00, fp_out);
	//	for (i = 0; i < 10; i++) {
	//		fputc(mdrname[i], fp_out);
	//		chksum.rrrr += mdrname[i];
	//		chksum.rrrr = chksum.rrrr % 255;
	//	}
	//	fputc(chksum.r[0], fp_out);
	//	// blank 2nd header
	//	chksum.rrrr = 0;
	//	for (i = 0; i < 14; i++) {
	//		fputc(0x00, fp_out);
	//		chksum.rrrr += 0x00;
	//		chksum.rrrr = chksum.rrrr % 255;
	//	}
	//	fputc(chksum.r[0], fp_out);
	//	chksum.rrrr = 0;
	//	for (i = 0; i < 512; i++) {
	//		fputc(0x00, fp_out);
	//		chksum.rrrr += 0x00;
	//		chksum.rrrr = chksum.rrrr % 255;
	//	}
	//	fputc(chksum.r[0], fp_out);
	//} while (sector > 0x00);
	//fputc(0x00, fp_out); // cartridge not write protected
	//sector = 0xfe;
	// add files to blank cartridge in interleaved format which leaves a sector between each sector written, which allows 
	// the drive to pick up the next sector quicker and as a result loads the game faster. After filling the drive it
	// loops back to the first unused sector
	// write run (a)	
	// 0x00 0xfa 0x00 0x05 0x5d 0xfa 0x00 0x00 0x00
	// basic or code
	// length in two bytes
	// start in two bytes
	// if basic then length again followed by 0x00 0x00
	// if code then 0xff 0xff 0xff 0xff
	// 
	uint8_t header[9];
	len.rrrr = mdrbln_len;
	header[0] = 0x00; //basic
	header[1] = header[5] = len.r[0];
	header[2] = header[6] = len.r[1];
	header[3] = start.r[0];
	header[4] = start.r[1];
	header[7] = 0x00;
	header[8] = 0x00;
	if (fwrite(header, sizeof(uint8_t), 9, fp_out) != 9) error(7);
	if (fwrite(mdrbln, sizeof(uint8_t), mdrbln_len, fp_out) != mdrbln_len) error(7);
	fclose(fp_out);
	//appendmdrf(mdrname, mdrfname, &fp_out, &sector, mdrbln, len, start, param, 0x00, u_sector);
	//
	// max adder size
	if (noc_launchigp_pos < 6912) {
		// copy prtbuf to ingap code 
		for (i = 0; i < noc_launchigp_begin; i++) comp[i + 287 - adder] = noc_launchigp[i];
		for (i = 0; i < delta; i++) comp[i + 287 - adder + noc_launchigp_begin] = main48k[49152 - delta + i];
		for (i = 0; i < noc_launchprt_len; i++) comp[i + 287 - adder + noc_launchigp_begin + delta] = main48k[6912 + i];
	}
	for (i = 0; i < noc_launchprt_len; i++) comp[i + 287 - noc_launchprt_len] = noc_launchprt[i];
	// sig
	//uint8_t sigfname[] = "-Z80onMDR-";
	//uint8_t sigfname[] = "-ZXPicoMD-";
	//len.rrrr = mdrsig_len;
	//appendmdrf(mdrname, sigfname, &fp_out, &sector, mdrsig, len, start, param, 0x00, u_sector);
	//
	//mdrfname[1] = mdrfname[2] = ' ';
	rrrr len_s;
	len_s.rrrr = simplelz(&main48k[0], &main48k[32768 + scrload_len], 6912);
	len_s.rrrr += scrload_len;
	for (i = 0; i < scrload_len; i++) main48k[32768 + i] = scrload[i]; // add m/c
	// write screen (b)
	fmdr[strlen(fmdr) -3] = '\0';
	strcat(fmdr, "0");
	if ((fp_out = fopen(fmdr, "wb")) == NULL) error(3); // cannot open screen file for write
	start.rrrr = 32201;
	header[0] = 0x03; //code
	header[1] = header[5] = len_s.r[0];
	header[2] = header[6] = len_s.r[1];
	header[3] = start.r[0];
	header[4] = start.r[1];
	header[7] = 0xff;
	header[8] = 0xff;
	if (fwrite(header, sizeof(uint8_t), 9, fp_out) != 9) error(7);
	if (fwrite(&main48k[32768], sizeof(uint8_t), len_s.rrrr, fp_out) != len_s.rrrr) error(7);
	fclose(fp_out);
	//mdrfname[0] = '0';
	//param.rrrr = 0xffff;
	//appendmdrf(mdrname, mdrfname, &fp_out, &sector, &main48k[32768], len_s, start, param, 0x03, u_sector);
	// write main
	fmdr[strlen(fmdr) - 1] = 'M';
	if ((fp_out = fopen(fmdr, "wb")) == NULL) error(3); // cannot open screen file for write
	start.rrrr = 65536 - cmsize.rrrr;
	header[1] = header[5] = cmsize.r[0];
	header[2] = header[6] = cmsize.r[1];
	header[3] = start.r[0];
	header[4] = start.r[1];
	if (fwrite(header, sizeof(uint8_t), 9, fp_out) != 9) error(7);
	if (fwrite(&comp[287 - adder], sizeof(uint8_t), cmsize.rrrr, fp_out) != cmsize.rrrr) error(7);
	fclose(fp_out);
	//mdrfname[0] = 'M';
	//appendmdrf(mdrname, mdrfname, &fp_out, &sector, &comp[287 - adder], cmsize, start, param, 0x03, u_sector);
	free(comp);
	// memory usage=49152
	// otek pages (c)
	if (otek) {
		fseek(fptmp, bank[4], SEEK_SET);
		if (fread(main48k, sizeof(uint8_t), 16384, fptmp) != 16384) error(7);
		len_s.rrrr = simplelz(main48k, &main48k[24576 + unpack_len], 16384);
		for (i = 0; i < unpack_len; i++) main48k[24576 + i] = unpack[i]; // add in unpacker
		len_s.rrrr += unpack_len;
		fmdr[strlen(fmdr) - 1] = '1';
		if ((fp_out = fopen(fmdr, "wb")) == NULL) error(3); // cannot open file for write
		start.rrrr = 32256 - unpack_len;
		header[1] = header[5] = len_s.r[0];
		header[2] = header[6] = len_s.r[1];
		header[3] = start.r[0];
		header[4] = start.r[1];
		if (fwrite(header, sizeof(uint8_t), 9, fp_out) != 9) error(7);
		if (fwrite(&main48k[24576], sizeof(uint8_t), len_s.rrrr, fp_out) != len_s.rrrr) error(7);
		fclose(fp_out);
		//mdrfname[0] = '1';
		//param.rrrr = 0xffff;
		//appendmdrf(mdrname, mdrfname, &fp_out, &sector, &main48k[24576], len_s, start, param, 0x03, u_sector);
		// page 3
		//mdrfname[0]++;
		main48k[24576] = 0x13;
		fseek(fptmp, bank[6], SEEK_SET);
		if (fread(main48k, sizeof(uint8_t), 16384, fptmp) != 16384) error(7);
		len_s.rrrr = simplelz(main48k, &main48k[24576 + 1], 16384);
		len_s.rrrr++;
		fmdr[strlen(fmdr) - 1] = '2';
		if ((fp_out = fopen(fmdr, "wb")) == NULL) error(3); // cannot open file for write
		start.rrrr = 32255; // don't need to replace the unpacker, just the page number
		header[1] = header[5] = len_s.r[0];
		header[2] = header[6] = len_s.r[1];
		header[3] = start.r[0];
		header[4] = start.r[1];
		if (fwrite(header, sizeof(uint8_t), 9, fp_out) != 9) error(7);
		if (fwrite(&main48k[24576], sizeof(uint8_t), len_s.rrrr, fp_out) != len_s.rrrr) error(7);
		fclose(fp_out);
		//appendmdrf(mdrname, mdrfname, &fp_out, &sector, &main48k[24576], len_s, start, param, 0x03, u_sector);
		// page 4
		//mdrfname[0]++;
		main48k[24576] = 0x14;
		fseek(fptmp, bank[7], SEEK_SET);
		if (fread(main48k, sizeof(uint8_t), 16384, fptmp) != 16384) error(7);
		len_s.rrrr = simplelz(main48k, &main48k[24576 + 1], 16384);
		len_s.rrrr++;
		fmdr[strlen(fmdr) - 1] = '3';
		if ((fp_out = fopen(fmdr, "wb")) == NULL) error(3); // cannot open file for write
		header[1] = header[5] = len_s.r[0];
		header[2] = header[6] = len_s.r[1];
		if (fwrite(header, sizeof(uint8_t), 9, fp_out) != 9) error(7);
		if (fwrite(&main48k[24576], sizeof(uint8_t), len_s.rrrr, fp_out) != len_s.rrrr) error(7);
		fclose(fp_out);
		//appendmdrf(mdrname, mdrfname, &fp_out, &sector, &main48k[24576], len_s, start, param, 0x03, u_sector);
		// page 6
		//mdrfname[0]++;
		main48k[24576] = 0x16;
		fseek(fptmp, bank[9], SEEK_SET);
		if (fread(main48k, sizeof(uint8_t), 16384, fptmp) != 16384) error(7);
		len_s.rrrr = simplelz(main48k, &main48k[24576 + 1], 16384);
		len_s.rrrr++;
		fmdr[strlen(fmdr) - 1] = '4';
		if ((fp_out = fopen(fmdr, "wb")) == NULL) error(3); // cannot open file for write
		header[1] = header[5] = len_s.r[0];
		header[2] = header[6] = len_s.r[1];
		if (fwrite(header, sizeof(uint8_t), 9, fp_out) != 9) error(7);
		if (fwrite(&main48k[24576], sizeof(uint8_t), len_s.rrrr, fp_out) != len_s.rrrr) error(7);
		fclose(fp_out);
		//appendmdrf(mdrname, mdrfname, &fp_out, &sector, &main48k[24576], len_s, start, param, 0x03, u_sector);
		// page 7
		//mdrfname[0]++;
		main48k[24576] = 0x17;
		fseek(fptmp, bank[10], SEEK_SET);
		if (fread(main48k, sizeof(uint8_t), 16384, fptmp) != 16384) error(7);
		len_s.rrrr = simplelz(main48k, &main48k[24576 + 1], 16384);
		len_s.rrrr++;
		fmdr[strlen(fmdr) - 1] = '5';
		if ((fp_out = fopen(fmdr, "wb")) == NULL) error(3); // cannot open screen file for write
		header[1] = header[5] = len_s.r[0];
		header[2] = header[6] = len_s.r[1];
		if (fwrite(header, sizeof(uint8_t), 9, fp_out) != 9) error(7);
		if (fwrite(&main48k[24576], sizeof(uint8_t), len_s.rrrr, fp_out) != len_s.rrrr) error(7);
		fclose(fp_out);
		//appendmdrf(mdrname, mdrfname, &fp_out, &sector, &main48k[24576], len_s, start, param, 0x03, u_sector);
	}
	free(main48k);
	//free(u_sector);
	fclose(fptmp);
	//fclose(fp_out);
	// all done
	return 0;
}
//decompress z80 snapshot routine
uint16_t dcz80(FILE** fp_in, uint8_t* out, uint16_t size) {
	uint16_t i = 0, k, j;
	uint8_t c;
	while (i < size) {
		c = fgetc(*fp_in);
		if (c == 0xed) { // is it 0xed [0]
			c = fgetc(*fp_in); // get next
			if (c == 0xed) { // is 2nd 0xed then a sequence
				j = fgetc(*fp_in); // counter into j
				c = fgetc(*fp_in);
				for (k = 0; k < j; k++) out[i++] = c;
			}
			else {
				out[i++] = 0xed;
				fseek(*fp_in, -1, SEEK_CUR); // back one
			}
		}
		else {
			out[i++] = c; // just copy
		}
	}
	return i;
}
//
uint32_t simplelz(uint8_t* fload, uint8_t* store, uint16_t filesize)
{
	int i;
	uint8_t* store_p, * store_c;

	int litsize = 1;
	int repsize, offset, repmax, offmax;
	store_c = store;
	store_p = store_c + 1;
	//
	i = 0;
	*store_p++ = fload[i++];
	do {
		// scan for sequence
		repmax = 2;
		if (i > 255) offset = i - 256; else offset = 0;
		do {
			repsize = 0;
			while (fload[offset + repsize] == fload[i + repsize] && i + repsize < filesize && repsize < 129) {
				repsize++;
			}
			if (repsize > repmax) {
				repmax = repsize;
				offmax = i - offset;
			}
			offset++;
		} while (offset < i && repmax < 129);
		if (repmax > 2) {
			if (litsize > 0) {
				*store_c = litsize - 1;
				store_c = store_p++;
				litsize = 0;
			}
			*store_p++ = offmax - 1; //1-256 -> 0-255
			*store_c = repmax + 126;
			store_c = store_p++;
			i += repmax;
		}
		else {
			litsize++;
			*store_p++ = fload[i++];
			if (litsize > 127) {
				*store_c = litsize - 1;
				store_c = store_p++;
				litsize = 0;
			}
		}
	} while (i < filesize);
	if (litsize > 0) {
		*store_c = litsize - 1;
		store_c = store_p++;
	}
	*store_c = 128;	// end marker
	return store_p - store;
}
//
uint16_t decompressf(uint8_t* store, uint16_t size, uint16_t filesize)
{
	int i, pos, spos;
	uint8_t c;
	pos = spos = 0;
	do {
		c = store[spos++];
		if (c > 128) {
			spos++;
			for (i = 0; i < c - 126; i++, pos++);
			if (pos - (filesize - size) > spos) return pos - (filesize - size) - spos;
		}
		else for (i = 0; i < c + 1; i++, pos++, spos++);
	} while (store[spos] != 128);
	return 0;
}
//
void error(uint8_t errorcode) {
	fprintf(stdout, "[E%02d]\n", errorcode);
	exit(errorcode);
}
