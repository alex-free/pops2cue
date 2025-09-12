/*
 * CUE2POPS - v2.3 - original tool by krHACKen // rebuilt by Bucanero
 * -----------------------------------------------------------------------
 *
 * I couldn't find the source code for v2.3 CUE2POPS tool and I wanted to run it on macOS,
 * so this is a reverse-engineered version based on a Linux binary disassembly (using Ghidra).
 *
 * For reference, the linux cue2pops binary was found as part of "OPL-POPS Game Manager (Beta)"
 * https://www.ps2-home.com/forum/viewtopic.php?t=2343
 *
 */

#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <stdlib.h>
#include <stdbool.h>

#define IOBUF_SIZE 0xA00000 // 10MB buffer size for caching BIN data in file output operations

FILE *file, *leech; //file is used for opening the input cue and the output file, leech is used for opening the BIN that's attached to the cue.
char *dumpaddr; // name/path of the BIN that is attached to the cue. Handled by the parser then altered if it doesn't contain the full path.
int sectorsize = 0; // Sector size
int gap_ptr = 0; // Indicates the location of the current INDEX 00 entry in the cue sheet
int fix_CDRWIN = 0; // Special CDRWIN pregap injection status
char LeadOut[10]; // Formatted Lead-Out MM:SS:FF
int dumpsize; // BIN (disc image) size
int sector_count; // Calculated number of sectors
int leadoutM; // Calculated Lead-Out MM:__:__
int leadoutS; // Calculated Lead-Out __:SS:__
int leadoutF; // Calculated Lead-Out __:__:FF
int track_count = 0;; // Number of "TRACK " occurrences in the cue
int pregap_count = 0;; // Number of "PREGAP" occurrences in the cue
int postgap_count = 0; // Number of "POSTGAP" occurrences in the cue
int daTrack_ptr = 0; // Indicates the location of the pregap that's between the data track and the first audio track

void system_pause(void)
{
#ifdef __WIN32
  puts("Press ENTER to continue...");
  getc(stdin);
#endif
  return;
}

void GetLeadOut(void)
{
  long sectors;
  
  sectors = (long)((postgap_count + pregap_count) * 0x96) + dumpsize / (long)sectorsize + 0x96;
  leadoutM = sectors / 0x1194;
  leadoutS = (sectors % 0x1194) / 0x4b;
  leadoutF = leadoutS * -0x4b + sectors % 0x1194;
  sector_count = (long)((postgap_count + pregap_count) * 0x96) + dumpsize / (long)sectorsize;
  sprintf(LeadOut,"%04d",leadoutM);
  sprintf(&LeadOut[4],"%02d",leadoutS);
  sprintf(&LeadOut[6],"%02d",leadoutF);
  LeadOut[0] = LeadOut[1] + LeadOut[0] * '\x10' + -0x30;
  LeadOut[1] = LeadOut[3] + LeadOut[2] * '\x10' + -0x30;
  LeadOut[2] = LeadOut[5] + LeadOut[4] * '\x10' + -0x30;
  LeadOut[3] = LeadOut[7] + LeadOut[6] * '\x10' + -0x30;
  LeadOut[4] = 0;
  LeadOut[5] = 0;
  LeadOut[6] = 0;
  LeadOut[7] = 0;
  return;
}

int main(int argc,char *argv[])

{
  int ret;
  int cue_ptr; // Indicates the location of the current INDEX 01 entry in the cue sheet
  int binary_count = 0; // Number of "BINARY" occurrences in the cue
  int index0_count = 0; // Number of "INDEX 00" occurrences in the cue
  int index1_count = 0; // Number of "INDEX 01" occurrences in the cue
  int wave_count = 0; // Number of "WAVE" occurrences in the cue
  int header_ptr = 0x14; // Integer that points to a location of the POPS header buffer. Decimal 20 is the start of the descriptor "A2"
  int mm; // Calculated and formatted MM:__:__ of the current index
  int ss; // Calculated and formatted __:SS:__ of the current index
  int ff; // Calculated and formatted __:__:FF of the current index
  int index0_adj = 0;
  int cuesize; // Size of the cue sheet
  char *ptr; // Pointer to the Track 01 type in the cue. Used to set the sector size, the disc type or to reject the cue
  size_t i; // Tracker
  char *cuebuf; // Buffer for the cue sheet
  char *headerbuf; // Buffer for the POPS header
  void *outbuf; // File I/O cache

  if (argc == 0) {
    ret = 0;
  }
  else {
    puts("\nBIN/CUE to POPStarter VCD conversion tool v2.3");
    puts("Last modified : 2016/11/03\n");
    for (i = strlen(argv[0]);
        (((0 < (long)i && (argv[0][i] != '\\')) && (argv[0][i] != '/'))
        && (argv[0][i] != ':')); i--) {
    }
    if (i != 0) {
      i++;
    }
    if (argc < 2) {
      puts("Error: No input file specified (cue sheet)\n");
      puts("Usage :");
      printf("%s input.cue output.VCD\n",*argv + i);
      puts("or");
      printf("%s input.cue\n",*argv + i);
      puts("or");
      printf("just drag your cue file to the %s icon.\n\n",*argv + i);
      system_pause();
      putchar(10);
      ret = 0;
    }
    else if (argc < 4) {
      if (strcmp(argv[1],"-help") != 0) {
        if (strcmp(argv[1],"/?") != 0) {
          if (strcmp(argv[1],"?") != 0) {
            printf("CUE Path -> %s\n",argv[1]);
            file = fopen(argv[1],"rb");
            if (file == NULL) {
              printf("Error: Cannot open %s\n\n",argv[1]);
              system_pause();
              putchar(10);
              ret = 0;
            }
            else {
              fseek(file, 0, SEEK_END);
              cuesize = (int)ftell(file);
              if (cuesize < 0xd351) {
                rewind(file);
                for (i = 0; (long)i < cuesize; i++) {
                  fseek(file, i, SEEK_SET);
                  if (fgetc(file) == 'F') {
                    if (fgetc(file) == 'I') {
                      if (fgetc(file) == 'L') {
                        if (fgetc(file) == 'E') {
                          if (fgetc(file) == ' ') {
                            if (fgetc(file) == '\"') break;
                          }
                        }
                      }
                    }
                  }
                }
                if (cuesize == (int)i) {
                  puts("Error: The cue sheet is not valid\n");
                  fclose(file);
                  system_pause();
                  putchar(10);
                  ret = 0;
                }
                else {
                  rewind(file);
                  cuebuf = (char *)malloc((long)(cuesize * 2));
                  fread(cuebuf,cuesize,1,file);
                  fclose(file);
                  ptr = strstr(cuebuf,"INDEX 01 ");
                  if (ptr == NULL) {
                    ptr = strstr(cuebuf,"INDEX 1 ");
                    if (ptr == NULL) {
                      puts("Error: The cue sheet is not valid\n");
                      free(cuebuf);
                      system_pause();
                      putchar(10);
                      ret = 0;
                      goto ExitApp;
                    }
                    ptr += 8;
                  }
                  else {
                    ptr += 9;
                  }
                  if ((*ptr == '0') && (ptr[1] == '0')) {
                    ptr = strstr(cuebuf,"FILE ");
                    ptr += 5;
                    if (*ptr == '\"') {
                      for (i = 0; (long)i < cuesize; i++) {
                        if (cuebuf[i] == '\"') {
                          cuebuf[i] = '\0';
                        }
                      }
                      ptr++;
                      dumpaddr = (char *)malloc((strlen(argv[1]) + strlen(ptr)) * 2);
                      for (i = strlen(ptr); 0 < (long)i; i--)
                      {
                        if (ptr[i] == '\\') {
                          strcpy(dumpaddr,ptr);
                          break;
                        }
                      }
                      if (i == 0) {
                        for (i = strlen(argv[1]);
                            (0 < (long)i && (argv[1][i] != '\\')); i--) {
                        }
                        strcpy(dumpaddr,argv[1]);
                        for (i = strlen(dumpaddr);
                            (0 < (long)i && (dumpaddr[i] != '\\')); i--) {
                        }
                        if ((long)i < 1) {
                          i = 0;
                          while( true ) {
                            if (strlen(dumpaddr) <= i) break;
                            dumpaddr[i] = '\0';
                            i++;
                          }
                        }
                        else {
                          while( true ) {
                            i++;
                            if (strlen(dumpaddr) <= i) break;
                            dumpaddr[i] = '\0';
                          }
                        }
                        i = strlen(dumpaddr);
                        strcpy(dumpaddr + i,ptr);
                        i = strlen(dumpaddr);
                        if (*argv[1] == '\"') {
                          dumpaddr[i] = '\"';
                        }
                        else {
                          dumpaddr[i] = '\0';
                        }
                      }
                      printf("BIN Path -> %s\n\n",dumpaddr);
                      leech = fopen(dumpaddr,"rb");
                      if (leech == NULL) {
                        printf("Error: Cannot open %s\n\n",dumpaddr);
                        free(cuebuf);
                        free(dumpaddr);
                        system_pause();
                        putchar(10);
                        ret = 0;
                      }
                      else {
                        fseeko(leech, 0, SEEK_END);
                        dumpsize = ftello(leech);
                        fclose(leech);
                        headerbuf = (void *)malloc(0x200000);
                        headerbuf[0] = 0x41;
                        headerbuf[2] = 0xa0;
                        headerbuf[7] = 1;
                        headerbuf[8] = 0x20;
                        headerbuf[0xc] = 0xa1;
                        headerbuf[0x16] = 0xa2;
                        for (i = 0;
                            ((long)i < cuesize &&
                            (((((cuebuf[i] != 'T' || (cuebuf[i + 1] != 'R')) ||
                               (cuebuf[i + 2] != 'A')) ||
                              (((cuebuf[i + 3] != 'C' || (cuebuf[i + 4] != 'K'))
                               || (cuebuf[i + 5] != ' ')))) ||
                             (((cuebuf[i + 6] != '0' || (cuebuf[i + 7] != '1')) &&
                              ((cuebuf[i + 6] != '1' || (cuebuf[i + 7] != ' ')))))
                             ))); i++) {
                        }
                        ptr = strstr(cuebuf + i,"TRACK 01 MODE2/2352");
                        if (ptr == NULL) {
                          ptr = strstr(cuebuf + i,"TRACK 1 MODE2/2352");
                        }
                        if (ptr == NULL) {
                          puts("Error: Looks like your game dump is not MODE2/2352, or the cue is invalid.\n");
                          free(cuebuf);
                          free(dumpaddr);
                          free(headerbuf);
                          system_pause();
                          putchar(10);
                          ret = 0;
                        }
                        else {
                          sectorsize = 0x930;
                          for (i = 0; (long)i < cuesize; i++) {
                            if (cuebuf[i] == ':') {
                              cuebuf[i] = '\0';
                            }
                            if (cuebuf[i] == '\r') {
                              cuebuf[i] = '\0';
                            }
                            if (cuebuf[i] == '\n') {
                              cuebuf[i] = '\0';
                            }
                            if (((((cuebuf[i] == 'T') && (cuebuf[i + 1] == 'R'))
                                 && (cuebuf[i + 2] == 'A')) &&
                                ((cuebuf[i + 3] == 'C' && (cuebuf[i + 4] == 'K')))
                                ) && (cuebuf[i + 5] == ' ')) {
                              track_count++;
                            }
                            if ((((cuebuf[i] == 'I') && (cuebuf[i + 1] == 'N')) &&
                                ((cuebuf[i + 2] == 'D' &&
                                 (((cuebuf[i + 3] == 'E' && (cuebuf[i + 4] == 'X')
                                   ) && (cuebuf[i + 5] == ' ')))))) &&
                               (((cuebuf[i + 6] == '0' && (cuebuf[i + 7] == '1'))
                                || ((cuebuf[i + 6] == '1' &&
                                    (cuebuf[i + 7] == ' ')))))) {
                              index1_count++;
                            }
                            if ((((cuebuf[i] == 'I') && (cuebuf[i + 1] == 'N')) &&
                                ((cuebuf[i + 2] == 'D' &&
                                 ((((cuebuf[i + 3] == 'E' &&
                                    (cuebuf[i + 4] == 'X')) &&
                                   (cuebuf[i + 5] == ' ')) &&
                                  (cuebuf[i + 6] == '0')))))) &&
                               ((cuebuf[i + 7] == '0' || (cuebuf[i + 7] == ' '))))
                            {
                              index0_count++;
                            }
                            if (((cuebuf[i] == 'B') && (cuebuf[i + 1] == 'I')) &&
                               ((cuebuf[i + 2] == 'N' &&
                                (((cuebuf[i + 3] == 'A' && (cuebuf[i + 4] == 'R'))
                                 && (cuebuf[i + 5] == 'Y')))))) {
                              binary_count++;
                            }
                            if (((cuebuf[i] == 'W') && (cuebuf[i + 1] == 'A')) &&
                               ((cuebuf[i + 2] == 'V' && (cuebuf[i + 3] == 'E'))))
                            {
                              wave_count++;
                            }
                            if (((cuebuf[i] == 'P') && (cuebuf[i + 1] == 'R')) &&
                               ((cuebuf[i + 2] == 'E' &&
                                (((cuebuf[i + 3] == 'G' && (cuebuf[i + 4] == 'A'))
                                 && (cuebuf[i + 5] == 'P')))))) {
                              pregap_count++;
                            }
                            if ((((cuebuf[i] == 'P') && (cuebuf[i + 1] == 'O')) &&
                                (((cuebuf[i + 2] == 'S' &&
                                  ((cuebuf[i + 3] == 'T' && (cuebuf[i + 4] == 'G')
                                   ))) && (cuebuf[i + 5] == 'A')))) &&
                               (cuebuf[i + 6] == 'P')) {
                              postgap_count++;
                            }
                          }
                          if (binary_count == 0) {
                            puts("Error: Unstandard cue sheet\n");
                            free(cuebuf);
                            free(dumpaddr);
                            free(headerbuf);
                            system_pause();
                            putchar(10);
                            ret = 0;
                          }
                          else if ((track_count == 0) || (track_count != index1_count)) {
                            puts("Error : Cannot count tracks\n");
                            free(cuebuf);
                            free(dumpaddr);
                            free(headerbuf);
                            system_pause();
                            putchar(10);
                            ret = 0;
                          }
                          else if ((binary_count == 1) && (wave_count == 0)) {
                            if ((pregap_count == 1) && (postgap_count == 0)) {
                              fix_CDRWIN = 1;
                            }
                            for (i = 0; (long)i < (long)track_count; i++) {
                              header_ptr = header_ptr + 10;
                              for (cue_ptr = 0; cue_ptr < cuesize; cue_ptr++) {
                                if (((((cuebuf[cue_ptr] == 'T') &&
                                      (cuebuf[cue_ptr + 1] == 'R')) &&
                                     (cuebuf[cue_ptr + 2] == 'A')) &&
                                    ((cuebuf[cue_ptr + 3] == 'C' &&
                                     (cuebuf[cue_ptr + 4] == 'K')))) &&
                                   (cuebuf[cue_ptr + 5] == ' ')) {
                                  cuebuf[cue_ptr] = '\0';
                                  if ((((cuebuf[cue_ptr + 0xd] == '2') ||
                                       (cuebuf[cue_ptr + 0xd] == '1')) ||
                                      (cuebuf[cue_ptr + 0xd] != 'O')) &&
                                     (cuebuf[cue_ptr + 7] != ' ')) {
                                    headerbuf[10] = 0x41;
                                    headerbuf[0x14] = 0x41;
                                    headerbuf[header_ptr] = 0x41;
                                  }
                                  else if ((cuebuf[cue_ptr + 0xd] == 'O') &&
                                          (cuebuf[cue_ptr + 7] != ' ')) {
                                    headerbuf[10] = 1;
                                    headerbuf[0x14] = 1;
                                    headerbuf[header_ptr] = 1;
                                  }
                                  else if (((cuebuf[cue_ptr + 0xc] == '2') ||
                                           ((cuebuf[cue_ptr + 0xc] == '1' ||
                                            (cuebuf[cue_ptr + 0xc] != 'O')))) &&
                                          (cuebuf[cue_ptr + 7] == ' ')) {
                                    headerbuf[10] = 0x41;
                                    headerbuf[0x14] = 0x41;
                                    headerbuf[header_ptr] = 0x41;
                                  }
                                  else if ((cuebuf[cue_ptr + 0xc] == 'O') &&
                                          (cuebuf[cue_ptr + 7] == ' ')) {
                                    headerbuf[10] = 1;
                                    headerbuf[0x14] = 1;
                                    headerbuf[header_ptr] = 1;
                                  }
                                  if (cuebuf[cue_ptr + 7] == ' ') {
                                    if (cuebuf[cue_ptr + 7] == ' ') {
                                      headerbuf[header_ptr + 2] =
                                           cuebuf[cue_ptr + 6] + -0x30;
                                      headerbuf[0x11] = cuebuf[cue_ptr + 6] + -0x30;
                                    }
                                  }
                                  else {
                                    headerbuf[header_ptr + 2] =
                                         cuebuf[cue_ptr + 7] +
                                         cuebuf[cue_ptr + 6] * '\x10' + -0x30;
                                    headerbuf[0x11] =
                                         cuebuf[cue_ptr + 7] +
                                         cuebuf[cue_ptr + 6] * '\x10' + -0x30;
                                  }
                                  break;
                                }
                              }
                              for (cue_ptr = 0; cue_ptr < cuesize; cue_ptr = cue_ptr + 1) {
                                if (((((cuebuf[cue_ptr] == 'I') &&
                                      (cuebuf[cue_ptr + 1] == 'N')) &&
                                     (cuebuf[cue_ptr + 2] == 'D')) &&
                                    (((cuebuf[cue_ptr + 3] == 'E' &&
                                      (cuebuf[cue_ptr + 4] == 'X')) &&
                                     ((cuebuf[cue_ptr + 5] == ' ' &&
                                      (cuebuf[cue_ptr + 6] == '0')))))) &&
                                   ((cuebuf[cue_ptr + 7] == '0' ||
                                    (cuebuf[cue_ptr + 7] == ' ')))) {
                                  gap_ptr = cue_ptr;
                                  index0_adj = (cuebuf[cue_ptr + 7] == ' ');
                                  cuebuf[cue_ptr] = '\0';
                                }
                                if ((((cuebuf[cue_ptr] == 'I') &&
                                     (cuebuf[cue_ptr + 1] == 'N')) &&
                                    (cuebuf[cue_ptr + 2] == 'D')) &&
                                   ((((cuebuf[cue_ptr + 3] == 'E' &&
                                      (cuebuf[cue_ptr + 4] == 'X')) &&
                                     (cuebuf[cue_ptr + 5] == ' ')) &&
                                    (((cuebuf[cue_ptr + 6] == '0' &&
                                      (cuebuf[cue_ptr + 7] == '1')) ||
                                     ((cuebuf[cue_ptr + 6] == '1' &&
                                      (cuebuf[cue_ptr + 7] == ' ')))))))) {
                                  cuebuf[cue_ptr] = '\0';
                                  if ((cuebuf[cue_ptr + 6] == '1') &&
                                     (cuebuf[cue_ptr + 7] == ' ')) {
                                    mm = cuebuf[cue_ptr + 9] + -0x30 +
                                            (cuebuf[cue_ptr + 8] + -0x30) * 0x10;
                                    ss = cuebuf[cue_ptr + 0xc] + -0x30 +
                                               (cuebuf[cue_ptr + 0xb] + -0x30) * 0x10;
                                    ff = cuebuf[cue_ptr + 0xf] + -0x30 +
                                               (cuebuf[cue_ptr + 0xe] + -0x30) * 0x10;
                                  }
                                  else {
                                    mm = cuebuf[cue_ptr + 10] + -0x30 +
                                            (cuebuf[cue_ptr + 9] + -0x30) * 0x10;
                                    ss = cuebuf[cue_ptr + 0xd] + -0x30 +
                                               (cuebuf[cue_ptr + 0xc] + -0x30) * 0x10;
                                    ff = cuebuf[cue_ptr + 0x10] + -0x30 +
                                               (cuebuf[cue_ptr + 0xf] + -0x30) * 0x10;
                                  }
                                  if (((daTrack_ptr == 0) && (headerbuf[10] == '\x01')) &&
                                     (gap_ptr != 0)) {
                                    if (index0_adj == 0) {
                                      daTrack_ptr = (long)(sectorsize *
                                                          (((cuebuf[gap_ptr + 9] + -0x30) *
                                                            10 + cuebuf[gap_ptr + 10] +
                                                                 -0x30) * 0x1194 +
                                                           ((cuebuf[gap_ptr + 0xc] + -0x30)
                                                            * 10 + cuebuf[gap_ptr + 0xd] +
                                                                   -0x30) * 0x4b +
                                                          (cuebuf[gap_ptr + 0xf] + -0x30) *
                                                          10 + cuebuf[gap_ptr + 0x10] +
                                                               -0x30));
                                    }
                                    else if (index0_adj == 1) {
                                      daTrack_ptr = (long)(sectorsize *
                                                          (((cuebuf[gap_ptr + 8] + -0x30) *
                                                            10 + cuebuf[gap_ptr + 9] + -0x30
                                                           ) * 0x1194 +
                                                           ((cuebuf[gap_ptr + 0xb] + -0x30)
                                                            * 10 + cuebuf[gap_ptr + 0xc] +
                                                                   -0x30) * 0x4b +
                                                          (cuebuf[gap_ptr + 0xe] + -0x30) *
                                                          10 + cuebuf[gap_ptr + 0xf] + -0x30
                                                          ));
                                    }
                                  }
                                  else if (((daTrack_ptr == 0) && (headerbuf[10] == '\x01')) &&
                                          (gap_ptr == 0)) {
                                    if (cuebuf[cue_ptr + 7] == ' ') {
                                      if (cuebuf[cue_ptr + 7] == ' ') {
                                        daTrack_ptr = (long)(sectorsize *
                                                            (((cuebuf[cue_ptr + 8] + -0x30)
                                                              * 10 + cuebuf[cue_ptr + 9] +
                                                                     -0x30) * 0x1194 +
                                                             ((cuebuf[cue_ptr + 0xb] +
                                                              -0x30) * 10 +
                                                             cuebuf[cue_ptr + 0xc] + -0x30)
                                                             * 0x4b + (cuebuf[cue_ptr + 0xe
                                                                               ] + -0x30) * 10 +
                                                                      cuebuf[cue_ptr + 0xf]
                                                                      + -0x30));
                                      }
                                    }
                                    else {
                                      daTrack_ptr = (long)(sectorsize *
                                                          (((cuebuf[cue_ptr + 9] + -0x30) *
                                                            10 + cuebuf[cue_ptr + 10] +
                                                                 -0x30) * 0x1194 +
                                                           ((cuebuf[cue_ptr + 0xc] + -0x30)
                                                            * 10 + cuebuf[cue_ptr + 0xd] +
                                                                   -0x30) * 0x4b +
                                                          (cuebuf[cue_ptr + 0xf] + -0x30) *
                                                          10 + cuebuf[cue_ptr + 0x10] +
                                                               -0x30));
                                    }
                                  }
                                  else if (((daTrack_ptr == 0) && (headerbuf[10] == 'A')) &&
                                          ((track_count - 1) == (int)i)) {
                                    daTrack_ptr = dumpsize;
                                  }
                                  headerbuf[header_ptr + 3] = (char)mm;
                                  headerbuf[header_ptr + 4] = (char)ss;
                                  headerbuf[header_ptr + 5] = (char)ff;
                                  headerbuf[header_ptr + 7] = (char)mm;
                                  headerbuf[header_ptr + 8] = (char)ss;
                                  headerbuf[header_ptr + 9] = (char)ff;
                                  if (gap_ptr != 0) {
                                    if (index0_adj == 1) {
                                      mm = cuebuf[gap_ptr + 9] + -0x30 +
                                              (cuebuf[gap_ptr + 8] + -0x30) * 0x10;
                                      ss = cuebuf[gap_ptr + 0xc] + -0x30 +
                                                 (cuebuf[gap_ptr + 0xb] + -0x30) * 0x10;
                                      ff = cuebuf[gap_ptr + 0xf] + -0x30 +
                                                 (cuebuf[gap_ptr + 0xe] + -0x30) * 0x10;
                                    }
                                    else {
                                      mm = cuebuf[gap_ptr + 10] + -0x30 +
                                              (cuebuf[gap_ptr + 9] + -0x30) * 0x10;
                                      ss = cuebuf[gap_ptr + 0xd] + -0x30 +
                                                 (cuebuf[gap_ptr + 0xc] + -0x30) * 0x10;
                                      ff = cuebuf[gap_ptr + 0x10] + -0x30 +
                                                 (cuebuf[gap_ptr + 0xf] + -0x30) * 0x10;
                                    }
                                    headerbuf[header_ptr + 3] = (char)mm;
                                    headerbuf[header_ptr + 4] = (char)ss;
                                    headerbuf[header_ptr + 5] = (char)ff;
                                  }
                                  if ((i == 0) ||
                                     (((((((headerbuf[header_ptr + 4] != '\b' &&
                                           (headerbuf[header_ptr + 4] != '\t')) &&
                                          (headerbuf[header_ptr + 4] != '\x18')) &&
                                         ((headerbuf[header_ptr + 4] != '\x19' &&
                                          (headerbuf[header_ptr + 4] != '(')))) &&
                                        ((headerbuf[header_ptr + 4] != ')' &&
                                         ((headerbuf[header_ptr + 4] != '8' &&
                                          (headerbuf[header_ptr + 4] != '9')))))) &&
                                       ((headerbuf[header_ptr + 4] != 'H' &&
                                        (headerbuf[header_ptr + 4] != 'I')))) ||
                                      ((headerbuf[header_ptr + 4] == 'X' &&
                                       (headerbuf[header_ptr + 4] == 'Y')))))) {
                                    if ((i == 0) ||
                                       ((headerbuf[header_ptr + 4] != 'X' &&
                                        (headerbuf[header_ptr + 4] != 'Y')))) {
                                      if (i != 0) {
                                        headerbuf[header_ptr + 4] =
                                             headerbuf[header_ptr + 4] + '\x02';
                                      }
                                    }
                                    else {
                                      if (headerbuf[header_ptr + 4] == 'X') {
                                        headerbuf[header_ptr + 4] = 0;
                                      }
                                      else if (headerbuf[header_ptr + 4] == 'Y') {
                                        headerbuf[header_ptr + 4] = 1;
                                      }
                                      if (headerbuf[header_ptr + 3] == '\t') {
                                        headerbuf[header_ptr + 3] = 0x10;
                                      }
                                      else if (headerbuf[header_ptr + 3] == '\x19') {
                                        headerbuf[header_ptr + 3] = 0x20;
                                      }
                                      else if (headerbuf[header_ptr + 3] == ')') {
                                        headerbuf[header_ptr + 3] = 0x30;
                                      }
                                      else if (headerbuf[header_ptr + 3] == '9') {
                                        headerbuf[header_ptr + 3] = 0x40;
                                      }
                                      else if (headerbuf[header_ptr + 3] == 'I') {
                                        headerbuf[header_ptr + 3] = 0x50;
                                      }
                                      else if (headerbuf[header_ptr + 3] == 'Y') {
                                        headerbuf[header_ptr + 3] = 0x60;
                                      }
                                      else if (headerbuf[header_ptr + 3] == 'i') {
                                        headerbuf[header_ptr + 3] = 0x70;
                                      }
                                      else if (headerbuf[header_ptr + 3] == 'y') {
                                        headerbuf[header_ptr + 3] = 0x80;
                                      }
                                      else if ((((headerbuf[header_ptr + 3] != '\t') &&
                                                (headerbuf[header_ptr + 3] != '\x19')) &&
                                               (headerbuf[header_ptr + 3] != ')')) &&
                                              (((headerbuf[header_ptr + 3] != '9' &&
                                                (headerbuf[header_ptr + 3] != 'I')) &&
                                               ((headerbuf[header_ptr + 3] != 'Y' &&
                                                ((headerbuf[header_ptr + 3] != 'i' &&
                                                 (headerbuf[header_ptr + 3] != 'y')))))))) {
                                        headerbuf[header_ptr + 3] =
                                             headerbuf[header_ptr + 3] + '\x01';
                                      }
                                    }
                                  }
                                  else {
                                    headerbuf[header_ptr + 4] =
                                         headerbuf[header_ptr + 4] + '\b';
                                  }
                                  if ((i == 0) ||
                                     ((((((headerbuf[header_ptr + 8] != '\b' &&
                                          (headerbuf[header_ptr + 8] != '\t')) &&
                                         (headerbuf[header_ptr + 8] != '\x18')) &&
                                        ((headerbuf[header_ptr + 8] != '\x19' &&
                                         (headerbuf[header_ptr + 8] != '(')))) &&
                                       ((((headerbuf[header_ptr + 8] != ')' &&
                                          ((headerbuf[header_ptr + 8] != '8' &&
                                           (headerbuf[header_ptr + 8] != '9')))) &&
                                         (headerbuf[header_ptr + 8] != 'H')) &&
                                        (headerbuf[header_ptr + 8] != 'I')))) ||
                                      ((headerbuf[header_ptr + 8] == 'X' &&
                                       (headerbuf[header_ptr + 8] == 'Y')))))) {
                                    if ((i == 0) ||
                                       ((headerbuf[header_ptr + 8] != 'X' &&
                                        (headerbuf[header_ptr + 8] != 'Y')))) {
                                      headerbuf[header_ptr + 8] =
                                           headerbuf[header_ptr + 8] + '\x02';
                                    }
                                    else {
                                      if (headerbuf[header_ptr + 8] == 'X') {
                                        headerbuf[header_ptr + 8] = 0;
                                      }
                                      else if (headerbuf[header_ptr + 8] == 'Y') {
                                        headerbuf[header_ptr + 8] = 1;
                                      }
                                      if (headerbuf[header_ptr + 7] == '\t') {
                                        headerbuf[header_ptr + 7] = 0x10;
                                      }
                                      else if (headerbuf[header_ptr + 7] == '\x19') {
                                        headerbuf[header_ptr + 7] = 0x20;
                                      }
                                      else if (headerbuf[header_ptr + 7] == ')') {
                                        headerbuf[header_ptr + 7] = 0x30;
                                      }
                                      else if (headerbuf[header_ptr + 7] == '9') {
                                        headerbuf[header_ptr + 7] = 0x40;
                                      }
                                      else if (headerbuf[header_ptr + 7] == 'I') {
                                        headerbuf[header_ptr + 7] = 0x50;
                                      }
                                      else if (headerbuf[header_ptr + 7] == 'Y') {
                                        headerbuf[header_ptr + 7] = 0x60;
                                      }
                                      else if (headerbuf[header_ptr + 7] == 'i') {
                                        headerbuf[header_ptr + 7] = 0x70;
                                      }
                                      else if (headerbuf[header_ptr + 7] == 'y') {
                                        headerbuf[header_ptr + 7] = 0x80;
                                      }
                                      else if (((((headerbuf[header_ptr + 7] != '\t') &&
                                                 (headerbuf[header_ptr + 7] != '\x19')) &&
                                                (headerbuf[header_ptr + 7] != ')')) &&
                                               ((headerbuf[header_ptr + 7] != '9' &&
                                                (headerbuf[header_ptr + 7] != 'I')))) &&
                                              ((headerbuf[header_ptr + 7] != 'Y' &&
                                               ((headerbuf[header_ptr + 7] != 'i' &&
                                                (headerbuf[header_ptr + 7] != 'y')))))) {
                                        headerbuf[header_ptr + 7] =
                                             headerbuf[header_ptr + 7] + '\x01';
                                      }
                                    }
                                  }
                                  else {
                                    headerbuf[header_ptr + 8] =
                                         headerbuf[header_ptr + 8] + '\b';
                                  }
                                  if (fix_CDRWIN == 1) {
                                    if (((i == 0) ||
                                        ((((((headerbuf[header_ptr + 4] != '\b' &&
                                             (headerbuf[header_ptr + 4] != '\t')) &&
                                            (headerbuf[header_ptr + 4] != '\x18')) &&
                                           (((headerbuf[header_ptr + 4] != '\x19' &&
                                             (headerbuf[header_ptr + 4] != '(')) &&
                                            ((headerbuf[header_ptr + 4] != ')' &&
                                             ((headerbuf[header_ptr + 4] != '8' &&
                                              (headerbuf[header_ptr + 4] != '9')))))))) &&
                                          (headerbuf[header_ptr + 4] != 'H')) &&
                                         (headerbuf[header_ptr + 4] != 'I')))) ||
                                       ((headerbuf[header_ptr + 4] == 'X' &&
                                        (headerbuf[header_ptr + 4] == 'Y')))) {
                                      if ((i == 0) ||
                                         ((headerbuf[header_ptr + 4] != 'X' &&
                                          (headerbuf[header_ptr + 4] != 'Y')))) {
                                        if (i != 0) {
                                          headerbuf[header_ptr + 4] =
                                               headerbuf[header_ptr + 4] + '\x02';
                                        }
                                      }
                                      else {
                                        if (headerbuf[header_ptr + 4] == 'X') {
                                          headerbuf[header_ptr + 4] = 0;
                                        }
                                        else if (headerbuf[header_ptr + 4] == 'Y') {
                                          headerbuf[header_ptr + 4] = 1;
                                        }
                                        if (headerbuf[header_ptr + 3] == '\t') {
                                          headerbuf[header_ptr + 3] = 0x10;
                                        }
                                        else if (headerbuf[header_ptr + 3] == '\x19') {
                                          headerbuf[header_ptr + 3] = 0x20;
                                        }
                                        else if (headerbuf[header_ptr + 3] == ')') {
                                          headerbuf[header_ptr + 3] = 0x30;
                                        }
                                        else if (headerbuf[header_ptr + 3] == '9') {
                                          headerbuf[header_ptr + 3] = 0x40;
                                        }
                                        else if (headerbuf[header_ptr + 3] == 'I') {
                                          headerbuf[header_ptr + 3] = 0x50;
                                        }
                                        else if (headerbuf[header_ptr + 3] == 'Y') {
                                          headerbuf[header_ptr + 3] = 0x60;
                                        }
                                        else if (headerbuf[header_ptr + 3] == 'i') {
                                          headerbuf[header_ptr + 3] = 0x70;
                                        }
                                        else if (headerbuf[header_ptr + 3] == 'y') {
                                          headerbuf[header_ptr + 3] = 0x80;
                                        }
                                        else if ((((headerbuf[header_ptr + 3] != '\t') &&
                                                  (headerbuf[header_ptr + 3] != '\x19')) &&
                                                 (headerbuf[header_ptr + 3] != ')')) &&
                                                (((headerbuf[header_ptr + 3] != '9' &&
                                                  (headerbuf[header_ptr + 3] != 'I')) &&
                                                 ((headerbuf[header_ptr + 3] != 'Y' &&
                                                  ((headerbuf[header_ptr + 3] != 'i' &&
                                                   (headerbuf[header_ptr + 3] != 'y')))))))) {
                                          headerbuf[header_ptr + 3] =
                                               headerbuf[header_ptr + 3] + '\x01';
                                        }
                                      }
                                    }
                                    else {
                                      headerbuf[header_ptr + 4] =
                                           headerbuf[header_ptr + 4] + '\b';
                                    }
                                    if ((i == 0) ||
                                       ((((((((headerbuf[header_ptr + 8] != '\b' &&
                                              (headerbuf[header_ptr + 8] != '\t')) &&
                                             (headerbuf[header_ptr + 8] != '\x18')) &&
                                            ((headerbuf[header_ptr + 8] != '\x19' &&
                                             (headerbuf[header_ptr + 8] != '(')))) &&
                                           (headerbuf[header_ptr + 8] != ')')) &&
                                          ((headerbuf[header_ptr + 8] != '8' &&
                                           (headerbuf[header_ptr + 8] != '9')))) &&
                                         ((headerbuf[header_ptr + 8] != 'H' &&
                                          (headerbuf[header_ptr + 8] != 'I')))) ||
                                        ((headerbuf[header_ptr + 8] == 'X' &&
                                         (headerbuf[header_ptr + 8] == 'Y')))))) {
                                      if ((i == 0) ||
                                         ((headerbuf[header_ptr + 8] != 'X' &&
                                          (headerbuf[header_ptr + 8] != 'Y')))) {
                                        if (i != 0) {
                                          headerbuf[header_ptr + 8] =
                                               headerbuf[header_ptr + 8] + '\x02';
                                        }
                                      }
                                      else {
                                        if (headerbuf[header_ptr + 8] == 'X') {
                                          headerbuf[header_ptr + 8] = 0;
                                        }
                                        else if (headerbuf[header_ptr + 8] == 'Y') {
                                          headerbuf[header_ptr + 8] = 1;
                                        }
                                        if (headerbuf[header_ptr + 7] == '\t') {
                                          headerbuf[header_ptr + 7] = 0x10;
                                        }
                                        else if (headerbuf[header_ptr + 7] == '\x19') {
                                          headerbuf[header_ptr + 7] = 0x20;
                                        }
                                        else if (headerbuf[header_ptr + 7] == ')') {
                                          headerbuf[header_ptr + 7] = 0x30;
                                        }
                                        else if (headerbuf[header_ptr + 7] == '9') {
                                          headerbuf[header_ptr + 7] = 0x40;
                                        }
                                        else if (headerbuf[header_ptr + 7] == 'I') {
                                          headerbuf[header_ptr + 7] = 0x50;
                                        }
                                        else if (headerbuf[header_ptr + 7] == 'Y') {
                                          headerbuf[header_ptr + 7] = 0x60;
                                        }
                                        else if (headerbuf[header_ptr + 7] == 'i') {
                                          headerbuf[header_ptr + 7] = 0x70;
                                        }
                                        else if (headerbuf[header_ptr + 7] == 'y') {
                                          headerbuf[header_ptr + 7] = 0x80;
                                        }
                                        else if ((((headerbuf[header_ptr + 7] != '\t') &&
                                                  (headerbuf[header_ptr + 7] != '\x19')) &&
                                                 (headerbuf[header_ptr + 7] != ')')) &&
                                                (((headerbuf[header_ptr + 7] != '9' &&
                                                  (headerbuf[header_ptr + 7] != 'I')) &&
                                                 ((headerbuf[header_ptr + 7] != 'Y' &&
                                                  ((headerbuf[header_ptr + 7] != 'i' &&
                                                   (headerbuf[header_ptr + 7] != 'y')))))))) {
                                          headerbuf[header_ptr + 7] =
                                               headerbuf[header_ptr + 7] + '\x01';
                                        }
                                      }
                                    }
                                    else {
                                      headerbuf[header_ptr + 8] =
                                           headerbuf[header_ptr + 8] + '\b';
                                    }
                                  }
                                  gap_ptr = 0;
                                  break;
                                }
                              }
                            }
                            free(cuebuf);
                            GetLeadOut();
                            headerbuf[0x1a] = LeadOut[0];
                            headerbuf[0x1b] = LeadOut[1];
                            headerbuf[0x1c] = LeadOut[2];
                            headerbuf[0x1d] = LeadOut[3];
                            memcpy(headerbuf + 0x408,&sector_count,4);
                            memcpy(headerbuf + 0x40c,&sector_count,4);
                            headerbuf[0x400] = 0x6b;
                            headerbuf[0x401] = 0x48;
                            headerbuf[0x402] = 0x6e;
                            headerbuf[0x403] = 0x22;
                            if (argc == 3) {
                              puts("Saving the virtual CD-ROM image. Please wait...");
                              file = fopen(argv[2],"wb");
                              if (file == NULL) {
                                printf("Error : Cannot write to %s\n\n",argv[2]);
                                free(dumpaddr);
                                free(headerbuf);
                                system_pause();
                                putchar(10);
                                ret = 0;
                              }
                              else {
                                fwrite(headerbuf,1,0x100000,file);
                                fclose(file);
                                free(headerbuf);
                                file = fopen(argv[2],"ab+");
                                if (file == NULL) {
                                  printf("Error : Cannot write to %s\n\n",argv[2]);
                                  free(dumpaddr);
                                  system_pause();
                                  putchar(10);
                                  ret = 0;
                                }
                                else {
                                  leech = fopen(dumpaddr,"rb");
                                  if (leech == NULL) {
                                    printf("Error: Cannot open %s\n\n",dumpaddr);
                                    free(dumpaddr);
                                    system_pause();
                                    putchar(10);
                                    ret = 0;
                                  }
                                  else {
                                    free(dumpaddr);
                                    outbuf = malloc(IOBUF_SIZE + 0x0400);
                                    for (i = 0; (long)i < dumpsize; i += IOBUF_SIZE) {
                                      if ((fix_CDRWIN == 1) &&
                                         (daTrack_ptr <= (long)(i + IOBUF_SIZE))) {
                                        fread(outbuf,(daTrack_ptr - (i + IOBUF_SIZE)) + IOBUF_SIZE,1,leech);
                                        fwrite(outbuf,(daTrack_ptr - (i + IOBUF_SIZE)) + IOBUF_SIZE,1,file);
                                        char padding[sectorsize * 300];
                                        fwrite(padding,(long)(sectorsize * 0x96),1, file);
                                        fread(outbuf,(i - daTrack_ptr) + IOBUF_SIZE,1,leech);
                                        fwrite(outbuf,(i - daTrack_ptr) + IOBUF_SIZE,1,file);
                                        fix_CDRWIN = 0;
                                      }
                                      else {
                                        fread(outbuf,IOBUF_SIZE,1,leech);
                                        if ((long)(i + IOBUF_SIZE) < dumpsize) {
                                          fwrite(outbuf,IOBUF_SIZE,1,file);
                                        }
                                        else {
                                          fwrite(outbuf,(dumpsize - (i + IOBUF_SIZE)) + IOBUF_SIZE,1,file);
                                        }
                                      }
                                    }
                                    fclose(leech);
                                    fclose(file);
                                    free(outbuf);
                                    puts("A POPS virtual CD-ROM image was saved as :");
                                    printf("%s\n\n",argv[2]);
                                    ret = 1;
                                  }
                                }
                              }
                            }
                            else {
                              for (i = strlen(argv[1]); i > 0; i--) {
                                if (((((argv[1][i] != '.') ||
                                         (argv[1][i + 1] != 'c')) ||
                                        (argv[1][i + 2] != 'u')) ||
                                       (argv[1][i + 3] != 'e')) &&
                                      (((argv[1][i] != '.' || (argv[1][i + 1] != 'C')
                                        ) || ((argv[1][i + 2] != 'U' ||
                                              (argv[1][i + 3] != 'E'))))))
                                              break;
                              }
                              if (i > 0) {
                                argv[1][i + 1] = 'V';
                                argv[1][i + 2] = 'C';
                                argv[1][i + 3] = 'D';
                              }
                              else {
                                i = strlen(argv[1]) - 1;
                                if (argv[1][i] == '\"') {
                                  argv[1][i + 4] = '\"';
                                }
                                else {
                                  argv[1][i + 4] = '\0';
                                }
                                argv[1][i] = '.';
                                argv[1][i + 1] = 'V';
                                argv[1][i + 2] = 'C';
                                argv[1][i + 3] = 'D';
                              }
                              i = strlen(argv[1]);
                              if ((((argv[1][i - 4] == '.') &&
                                   (argv[1][i - 3] == 'V')) &&
                                  ((argv[1][i - 2] == 'C' &&
                                   (argv[1][i - 1] == 'D')))) ||
                                 ((((argv[1][i - 5] == '.' &&
                                    (argv[1][i - 4] == 'V')) &&
                                   (argv[1][i - 3] == 'C')) &&
                                  ((argv[1][i - 2] == 'D' &&
                                   (argv[1][i - 1] == '\"')))))) {
                                puts("Saving the virtual CD-ROM image. Please wait...");
                                file = fopen(argv[1],"wb");
                                if (file == NULL) {
                                  printf("Error : Cannot write to %s\n\n",argv[1]);
                                  free(dumpaddr);
                                  free(headerbuf);
                                  system_pause();
                                  putchar(10);
                                  ret = 0;
                                }
                                else {
                                  fwrite(headerbuf,1,0x100000,file);
                                  fclose(file);
                                  free(headerbuf);
                                  file = fopen(argv[1],"ab+");
                                  if (file == NULL) {
                                    printf("Error : Cannot write to %s\n\n",argv[1]);
                                    free(dumpaddr);
                                    system_pause();
                                    putchar(10);
                                    ret = 0;
                                  }
                                  else {
                                    leech = fopen(dumpaddr,"rb");
                                    if (leech == NULL) {
                                      printf("Error: Cannot open %s\n\n",dumpaddr);
                                      free(dumpaddr);
                                      system_pause();
                                      putchar(10);
                                      ret = 0;
                                    }
                                    else {
                                      free(dumpaddr);
                                      outbuf = malloc(0xa00400);
                                      for (i = 0; (long)i < dumpsize; i += IOBUF_SIZE) {
                                        if ((fix_CDRWIN == 1) &&
                                           (daTrack_ptr <= (long)(i + IOBUF_SIZE))) {
                                          fread(outbuf,(daTrack_ptr - (i + IOBUF_SIZE)) + IOBUF_SIZE,1,leech);
                                          fwrite(outbuf,(daTrack_ptr - (i + IOBUF_SIZE)) + IOBUF_SIZE,1,file);
                                          char padding[sectorsize * 300];
                                          fwrite(padding,(long)(sectorsize * 0x96),1, file);
                                          fread(outbuf,(i - daTrack_ptr) + IOBUF_SIZE,1,leech);
                                          fwrite(outbuf,(i - daTrack_ptr) + IOBUF_SIZE,1,file);
                                          fix_CDRWIN = 0;
                                        }
                                        else {
                                          fread(outbuf,IOBUF_SIZE,1,leech);
                                          if ((long)(i + IOBUF_SIZE) < dumpsize) {
                                            fwrite(outbuf,IOBUF_SIZE,1,file);
                                          }
                                          else {
                                            fwrite(outbuf,(dumpsize - (i + IOBUF_SIZE)) + IOBUF_SIZE,1,file);
                                          }
                                        }
                                      }
                                      fclose(leech);
                                      fclose(file);
                                      free(outbuf);
                                      puts("A POPS virtual CD-ROM image was saved as :");
                                      printf("%s\n\n",argv[1]);
                                      ret = 1;
                                    }
                                  }
                                }
                              }
                              else {
                                puts("Unexpected error\n");
                                free(dumpaddr);
                                free(headerbuf);
                                system_pause();
                                putchar(10);
                                ret = 0;
                              }
                            }
                          }
                          else {
                            puts("Error: Cue sheets of splitted dumps aren\'t supported");
                            puts("       CDmage and IsoBuster can merge tracks in a single BIN/CUE.\n ");
                            free(cuebuf);
                            free(dumpaddr);
                            free(headerbuf);
                            system_pause();
                            putchar(10);
                            ret = 0;
                          }
                        }
                      }
                    }
                    else {
                      puts("Error: The cue sheet is not valid\n");
                      free(cuebuf);
                      system_pause();
                      putchar(10);
                      ret = 0;
                    }
                  }
                  else {
                    puts("Error: The cue sheet is not valid\n");
                    free(cuebuf);
                    system_pause();
                    putchar(10);
                    ret = 0;
                  }
                }
              }
              else {
                puts("Error: The cue sheet is not valid\n");
                fclose(file);
                system_pause();
                putchar(10);
                ret = 0;
              }
            }
            goto ExitApp;
          }
        }
      }
      puts("Hello! I convert BIN+CUE disc images to the POPS format.");
      puts("The BIN has to be a raw MODE2/2352 dump.");
      puts("The CUE has to be a standard ASCII cuesheet.");
      puts("Exotic files like NRG, CCD, ISO, GI... are not supported.");
      puts("Splitted dumps (with separate tracks) are not supported.");
      puts("High density dumps (aka merged/combined discs) are supported.");
      puts("Old extra commands gap++, gap--, vmode, trainer, whatever, no longer exist.\n");
      puts("Usage:");
      printf("\"%s\" \"input_file\" \"output_file\"\n\n",*argv + i);
      puts("input_file must be an ASCII cuesheet (CUE) which points to a Mode 2 Form 1 CD-ROM image.");
      puts("output_file is the filename of the VCD you want to create (optional).\n");
      system_pause();
      putchar(10);
      ret = 0;
    }
    else {
      puts("Error: Too many arguments\n");
      puts("Usage :");
      printf("%s input.cue output.VCD\n",*argv + i);
      puts("or");
      printf("%s input.cue\n\n",*argv + i);
      system_pause();
      putchar(10);
      ret = 0;
    }
  }
ExitApp:
  return ret;
}
