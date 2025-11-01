/*
 * POPS2CUE - v1.0 - original tool by krHACKen // rebuilt by Bucanero
 * -----------------------------------------------------------------------
 *
 * I couldn't find the source code for this POPS2CUE tool and I wanted to run it on macOS,
 * so this is a reverse-engineered version based on a Linux binary disassembly (using Ghidra).
 *
 * For reference, the linux pops2cue binary was found as part of "OPL-POPS Game Manager (Beta)"
 * https://www.ps2-home.com/forum/viewtopic.php?t=2343
 *
 */

#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <stdlib.h>
#include <stdbool.h>


int converter(char input, int export, char *output)
{
  int ret = 0;
  uint8_t buf[4];

  if (export == 0) {
    sprintf((char *)&buf,"%02X",(uint32_t)(uint8_t)input);
    ret = (buf[0] - 0x30) * 10 + (buf[1] - 0x30);
  }
  else {
    sprintf(output, "%02X", (uint32_t)(uint8_t)input);
  }

  return ret;
}

void usage(const char* app_bin, int argc, const char* arg_opt)
{
  if (argc == 1) {
    printf("Error : Unrecognized option \"%s\"\n\n", arg_opt);
  }
  if (argc == 2) {
    puts("Error : Too many options\n");
  }
  printf("Usage :\n%s <input_VCD> <option1> <option2>\n\n",app_bin);
  puts("Options are :");
  puts("-nobin      : Do not save the disc image (saves the cuesheet only)");
  puts("-noindex00  : Do not save INDEX 00 entries in the cuesheet\n");
  puts("This program accepts VCDs that were made with CUE2POPS v2.0\n\n");
  exit(1);
}

int main(int argc, char *argv[])
{
  bool noBin = false;
  bool noIndex = false;
  int res;
  int ret;
  FILE *fVCD;
  FILE *fOut;
  int i, j, k;
  int pos;
  char *vcd_hdr;
  long bin_size;
  char buffer[8];

  puts("\nPOPS2CUE v1.0, a POPS VCD to BIN+CUE converter\n");
  if (argc == 1) {
    usage(argv[0], 0, 0);
  }
  else if (((argc == 2) || (argc == 3)) || (argc == 4)) {
    if (argc == 3) {
      res = strcmp((char *)argv[2],"nobin");
      if ((((res == 0) || (res = strcmp((char *)argv[2],"/nobin"), res == 0)) ||
          ((res = strcmp((char *)argv[2],"-nobin"), res == 0 ||
           ((res = strcmp((char *)argv[2],"noBIN"), res == 0 ||
            (res = strcmp((char *)argv[2],"/noBIN"), res == 0)))))) ||
         (res = strcmp((char *)argv[2],"-noBIN"), res == 0)) {
        noBin = true;
      }
      else {
        res = strcmp((char *)argv[2],"noindex00");
        if (((((res != 0) && (res = strcmp((char *)argv[2],"/noindex00"), res != 0)) &&
             (res = strcmp((char *)argv[2],"-noindex00"), res != 0)) &&
            ((res = strcmp((char *)argv[2],"noINDEX00"), res != 0 &&
             (res = strcmp((char *)argv[2],"/noINDEX00"), res != 0)))) &&
           (res = strcmp((char *)argv[2],"-noINDEX00"), res != 0)) {
          usage(argv[0], 1, argv[2]);
        }
        noIndex = true;
      }
    }
    if (argc == 4) {
      res = strcmp((char *)argv[3],"nobin");
      if (((res == 0) || (res = strcmp((char *)argv[3],"/nobin"), res == 0)) ||
         ((res = strcmp((char *)argv[3],"-nobin"), res == 0 ||
          (((res = strcmp((char *)argv[3],"noBIN"), res == 0 ||
            (res = strcmp((char *)argv[3],"/noBIN"), res == 0)) ||
           (res = strcmp((char *)argv[3],"-noBIN"), res == 0)))))) {
        noBin = true;
      }
      else {
        res = strcmp((char *)argv[3],"noindex00");
        if ((((res != 0) && (res = strcmp((char *)argv[3],"/noindex00"), res != 0)) &&
            (res = strcmp((char *)argv[3],"-noindex00"), res != 0)) &&
           (((res = strcmp((char *)argv[3],"noINDEX00"), res != 0 &&
             (res = strcmp((char *)argv[3],"/noINDEX00"), res != 0)) &&
            (res = strcmp((char *)argv[3],"-noINDEX00"), res != 0)))) {
          usage(argv[0],1,argv[3]);
        }
        noIndex = true;
      }
    }
    printf("Reading the VCD header...");
    fVCD = fopen((char *)argv[1],"rb");
    if (fVCD == NULL) {
      printf("Error : Cannot open %s\n\n",argv[1]);
      exit(1);
    }
    else {
      fseeko(fVCD, 0, SEEK_END);
      bin_size = ftello(fVCD);
      if (bin_size < 0x10a560) {
        puts("\nError : Input file isn\'t a POPS VCD\n");
        fclose(fVCD);
        exit(1);
      }
      else {
        if (noBin) {
          vcd_hdr = (char *)malloc(0x810);
        }
        else {
          vcd_hdr = (char *)malloc(0xa00400);
        }
        rewind(fVCD);
        fread(vcd_hdr,0x410,1,fVCD);
        if (noBin) {
          fclose(fVCD);
        }
        else {
          fseeko(fVCD, 0x100000, SEEK_SET);
          bin_size -= 0x100000;
        }
        if ((((vcd_hdr[0x408] == vcd_hdr[0x40c]) && (vcd_hdr[0x409] == vcd_hdr[0x40d])) &&
            (vcd_hdr[0x40a] == vcd_hdr[0x40e])) && (vcd_hdr[0x40b] == vcd_hdr[0x40f])) {
          if (vcd_hdr[2] == -0x60) {
            if (vcd_hdr[0xc] == -0x5f) {
              if (vcd_hdr[0x16] == -0x5e) {
                if (*vcd_hdr == 'A') {
                  if (vcd_hdr[8] == ' ') {
                    if (vcd_hdr[7] == '\x01') {
                      if (((vcd_hdr[0x400] == 'k') && (vcd_hdr[0x401] == 'H')) &&
                         (vcd_hdr[0x402] == 'n')) {
                        if ((uint8_t)vcd_hdr[0x403] < 0x20) {
                          if (!noBin) {
                            fclose(fVCD);
                          }
                          puts("\nError : Invalid VCD");
                          puts("        You cannot convert a VCD that has been made by CUE2POPS v1.X\n");
                          free(vcd_hdr);
                          exit(1);
                        }
                        else {
                          printf(" Done\nReverting the timestamps...");
                          res = converter((int)vcd_hdr[0x11],0,0);
                          pos = 0x1e;
                          for (i = 0; i != res; i++) {
                            if (!noIndex) {
                              if (((i == 0) ||
                                  ((((vcd_hdr[pos + 4] != 'P' &&
                                     (vcd_hdr[pos + 4] != 'Q')) &&
                                    ((vcd_hdr[pos + 4] != '\x10' &&
                                     (((((vcd_hdr[pos + 4] != '\x11' &&
                                         (vcd_hdr[pos + 4] != ' ')) &&
                                        (vcd_hdr[pos + 4] != '!')) &&
                                       ((vcd_hdr[pos + 4] != '0' &&
                                        (vcd_hdr[pos + 4] != '1')))) &&
                                      (vcd_hdr[pos + 4] != '@')))))) &&
                                   (vcd_hdr[pos + 4] != 'A')))) ||
                                 ((vcd_hdr[pos + 4] == '\0' &&
                                  (vcd_hdr[pos + 4] == '\x01')))) {
                                if ((i == 0) ||
                                   ((vcd_hdr[pos + 4] != '\0' &&
                                    (vcd_hdr[pos + 4] != '\x01')))) {
                                  if (i != 0) {
                                    vcd_hdr[pos + 4] = vcd_hdr[pos + 4] - 2;
                                  }
                                }
                                else {
                                  if (vcd_hdr[pos + 4] == '\0') {
                                    vcd_hdr[pos + 4] = 'X';
                                  }
                                  if (vcd_hdr[pos + 4] == '\x01') {
                                    vcd_hdr[pos + 4] = 'Y';
                                  }
                                  if (vcd_hdr[pos + 3] == '\x10') {
                                    vcd_hdr[pos + 3] = '\t';
                                  }
                                  if (vcd_hdr[pos + 3] == ' ') {
                                    vcd_hdr[pos + 3] = '\x19';
                                  }
                                  if (vcd_hdr[pos + 3] == '0') {
                                    vcd_hdr[pos + 3] = ')';
                                  }
                                  if (vcd_hdr[pos + 3] == '@') {
                                    vcd_hdr[pos + 3] = '9';
                                  }
                                  if (vcd_hdr[pos + 3] == 'P') {
                                    vcd_hdr[pos + 3] = 'I';
                                  }
                                  if (vcd_hdr[pos + 3] == '`') {
                                    vcd_hdr[pos + 3] = 'Y';
                                  }
                                  if (vcd_hdr[pos + 3] == 'p') {
                                    vcd_hdr[pos + 3] = 'i';
                                  }
                                  if (vcd_hdr[pos + 3] == -0x80) {
                                    vcd_hdr[pos + 3] = 'y';
                                  }
                                  if (vcd_hdr[pos + 3] == -0x70) {
                                    vcd_hdr[pos + 3] = -0x77;
                                  }
                                  if ((((vcd_hdr[pos + 3] != -0x70) &&
                                       (vcd_hdr[pos + 3] != '\x10')) &&
                                      (vcd_hdr[pos + 3] != ' ')) &&
                                     ((((vcd_hdr[pos + 3] != '0' &&
                                        (vcd_hdr[pos + 3] != '@')) &&
                                       ((vcd_hdr[pos + 3] != 'P' &&
                                        ((vcd_hdr[pos + 3] != '`' &&
                                         (vcd_hdr[pos + 3] != 'p')))))) &&
                                      (vcd_hdr[pos + 3] != -0x80)))) {
                                    vcd_hdr[pos + 3] = vcd_hdr[pos + 3] - 1;
                                  }
                                }
                              }
                              else {
                                vcd_hdr[pos + 4] = vcd_hdr[pos + 4] - 8;
                              }
                            }
                            if (((i == 0) ||
                                (((((vcd_hdr[pos + 8] != 'P' &&
                                    (vcd_hdr[pos + 8] != 'Q')) &&
                                   (vcd_hdr[pos + 8] != '\x10')) &&
                                  ((vcd_hdr[pos + 8] != '\x11' &&
                                   (vcd_hdr[pos + 8] != ' ')))) &&
                                 ((((vcd_hdr[pos + 8] != '!' &&
                                    ((vcd_hdr[pos + 8] != '0' &&
                                     (vcd_hdr[pos + 8] != '1')))) &&
                                   (vcd_hdr[pos + 8] != '@')) &&
                                  (vcd_hdr[pos + 8] != 'A')))))) ||
                               ((vcd_hdr[pos + 8] == '\0' &&
                                (vcd_hdr[pos + 8] == '\x01')))) {
                              if ((i == 0) ||
                                 ((vcd_hdr[pos + 8] != '\0' &&
                                  (vcd_hdr[pos + 8] != '\x01')))) {
                                vcd_hdr[pos + 8] = vcd_hdr[pos + 8] - 2;
                              }
                              else {
                                if (vcd_hdr[pos + 8] == '\0') {
                                  vcd_hdr[pos + 8] = 'X';
                                }
                                if (vcd_hdr[pos + 8] == '\x01') {
                                  vcd_hdr[pos + 8] = 'Y';
                                }
                                if (vcd_hdr[pos + 7] == '\x10') {
                                  vcd_hdr[pos + 7] = '\t';
                                }
                                if (vcd_hdr[pos + 7] == ' ') {
                                  vcd_hdr[pos + 7] = '\x19';
                                }
                                if (vcd_hdr[pos + 7] == '0') {
                                  vcd_hdr[pos + 7] = ')';
                                }
                                if (vcd_hdr[pos + 7] == '@') {
                                  vcd_hdr[pos + 7] = '9';
                                }
                                if (vcd_hdr[pos + 7] == 'P') {
                                  vcd_hdr[pos + 7] = 'I';
                                }
                                if (vcd_hdr[pos + 7] == '`') {
                                  vcd_hdr[pos + 7] = 'Y';
                                }
                                if (vcd_hdr[pos + 7] == 'p') {
                                  vcd_hdr[pos + 7] = 'i';
                                }
                                if (vcd_hdr[pos + 7] == -0x80) {
                                  vcd_hdr[pos + 7] = 'y';
                                }
                                if (vcd_hdr[pos + 7] == -0x70) {
                                  vcd_hdr[pos + 7] = -0x77;
                                }
                                if (((((vcd_hdr[pos + 7] != -0x70) &&
                                      (vcd_hdr[pos + 7] != '\x10')) &&
                                     (vcd_hdr[pos + 7] != ' ')) &&
                                    ((vcd_hdr[pos + 7] != '0' &&
                                     (vcd_hdr[pos + 7] != '@')))) &&
                                   (((vcd_hdr[pos + 7] != 'P' &&
                                     ((vcd_hdr[pos + 7] != '`' &&
                                      (vcd_hdr[pos + 7] != 'p')))) &&
                                    (vcd_hdr[pos + 7] != -0x80)))) {
                                  vcd_hdr[pos + 7] = vcd_hdr[pos + 7] - 1;
                                }
                              }
                            }
                            else {
                              vcd_hdr[pos + 8] = vcd_hdr[pos + 8] - 8;
                            }
                            pos = pos + 10;
                          }
                          puts(" Done");
                          i = strlen((char *)argv[1]);
                          while ((0 < i &&
                                 ((((*(char *)(i + argv[1]) != '.' ||
                                    (*(char *)(argv[1] + i + 1) != 'V')) ||
                                   (*(char *)(argv[1] + i + 2) != 'C')) ||
                                  (*(char *)(argv[1] + i + 3) != 'D'))))) {
                            i--;
                          }
                          if (i < 0) {
                            k = strlen((char *)argv[1]);
                            if (*(char *)argv[1] == '\"') {
                              *(char *)(argv[1] + k + 4) = 0x22;
                            }
                            *(char *)(k + argv[1]) = 0x2e;
                            *(char *)(argv[1] + k + 1) = 99;
                            *(char *)(argv[1] + k + 2) = 0x75;
                            *(char *)(argv[1] + k + 3) = 0x65;
                          }
                          else {
                            *(char *)(argv[1] + i + 1) = 99;
                            *(char *)(argv[1] + i + 2) = 0x75;
                            *(char *)(argv[1] + i + 3) = 0x65;
                          }
                          printf("Saving the CUE file in the VCD directory...");
                          fOut = fopen((char *)argv[1],"wb");
                          if (fOut == NULL) {
                            printf("Error : Cannot write to %s\n\n",argv[1]);
                            if (!noBin) {
                              fclose(fVCD);
                            }
                            free(vcd_hdr);
                            exit(1);
                          }
                          else {
                            k = strlen((char *)argv[1]);
                            *(char *)(argv[1] + k + -3) = 0x62;
                            *(char *)(argv[1] + k + -2) = 0x69;
                            *(char *)(argv[1] + k + -1) = 0x6e;
                            fwrite("FILE ",1,5,fOut);
                            if (*(char *)argv[1] != '\"') {
                              fwrite("\x22", 1, 1, fOut);
                            }
                            for (j = strlen((char *)argv[1]); 0 < j; j--) {
                              if (((*(char *)(argv[1] + j + -1) == ':') ||
                                  (*(char *)(argv[1] + j + -1) == '/')) ||
                                 (*(char *)(argv[1] + j + -1) == '\\')) {
                                fwrite((void *)(argv[1] + j), 1, (long)(k - j),fOut);
                                break;
                              }
                            }
                            if (j == 0) {
                              fwrite((void *)argv[1],1,k,fOut);
                            }
                            if (*(char *)argv[1] != '\"') {
                              fwrite("\x22", 1, 1, fOut);
                            }
                            fwrite(" BINARY\n   TRACK 01 MODE2/2352\n   INDEX 01 00:00:00\n",1,0x34, fOut);
                            pos = 0x28;
                            for (i = 1; i != res; i++) {
                              fwrite("   TRACK \n",1,9,fOut);
                              converter((int)vcd_hdr[pos + 2],1,buffer);
                              fwrite(buffer,1,2,fOut);
                              fwrite("\x20", 1, 1,fOut);
                              if (vcd_hdr[pos] == 'A') {
                                fwrite("MODE2/2352",1,10,fOut);
                              }
                              else {
                                fwrite("AUDIO",1,5,fOut);
                              }
                              if ((!noIndex) &&
                                 (vcd_hdr[pos + 4] != vcd_hdr[pos + 8])) {
                                fwrite("\n   INDEX 00 ",1,0xd,fOut);
                                converter((int)vcd_hdr[pos + 3],1,buffer);
                                fwrite(buffer,1,2,fOut);
                                fwrite(":",1,1,fOut);
                                converter((int)vcd_hdr[pos + 4],1,buffer);
                                fwrite(buffer,1,2,fOut);
                                fwrite(":", 1, 1, fOut);
                                converter((int)vcd_hdr[pos + 5],1,buffer);
                                fwrite(buffer,1,2,fOut);
                              }
                              fwrite("\n   INDEX 01 ",1,0xd,fOut);
                              converter((int)vcd_hdr[pos + 7],1,buffer);
                              fwrite(buffer,1,2,fOut);
                              fwrite(":", 1, 1, fOut);
                              converter((int)vcd_hdr[pos + 8],1,buffer);
                              fwrite(buffer,1,2,fOut);
                              fwrite(":", 1, 1, fOut);
                              converter((int)vcd_hdr[pos + 9],1,buffer);
                              fwrite(buffer,1,2,fOut);
                              if (res + -1 != i) {
                                fwrite("\x0a", 1, 1, fOut);
                              }
                              pos += 10;
                            }
                            fclose(fOut);
                            if (!noBin) {
                              puts(" Done");
                              printf("Saving the BIN file in the VCD directory...");
                              memset(vcd_hdr,0,0xa00000);
                              fOut = fopen((char *)argv[1],"wb");
                              if (fOut == NULL) {
                                printf("Error : Cannot write to %s\n\n",argv[1]);
                                fclose(fVCD);
                                free(vcd_hdr);
                                exit(1);
                              }
                              for (i = 0; i < bin_size; i += 0xa00000) {
                                fread(vcd_hdr,0xa00000,1,fVCD);
                                if (i + 0xa00000 < bin_size) {
                                  fwrite(vcd_hdr,0xa00000,1,fOut);
                                }
                                else {
                                  fwrite(vcd_hdr,(bin_size - (i + 0xa00000)) + 0xa00000,1, fOut);
                                }
                              }
                              fclose(fOut);
                              fclose(fVCD);
                            }
                            free(vcd_hdr);
                            puts(" Done\n");
                            exit(0);
                          }
                        }
                      }
                      else {
                        if (!noBin) {
                          fclose(fVCD);
                        }
                        puts("\nError : Invalid VCD");
                        puts("        The disc image hasn\'t been converted by CUE2POPS\n");
                        free(vcd_hdr);
                        exit(1);
                      }
                    }
                    else {
                      if (!noBin) {
                        fclose(fVCD);
                      }
                      puts("\nError : Invalid VCD");
                      puts("        The first track isn\'t TRACK 01\n");
                      free(vcd_hdr);
                      exit(1);
                    }
                  }
                  else {
                    if (!noBin) {
                      fclose(fVCD);
                    }
                    puts("\nError : Invalid VCD");
                    puts("        The disc type isn\'t CD-XA001\n");
                    free(vcd_hdr);
                    exit(1);
                  }
                }
                else {
                  if (!noBin) {
                    fclose(fVCD);
                  }
                  free(vcd_hdr);
                  puts("\nError : Invalid VCD");
                  puts("        The first track must be a DATA track\n");
                  exit(1);
                }
              }
              else {
                if (!noBin) {
                  fclose(fVCD);
                }
                puts("\nError : Invalid VCD");
                puts("        The disc lead-in/out array wasn\'t found\n");
                free(vcd_hdr);
                exit(1);
              }
            }
            else {
              if (!noBin) {
                fclose(fVCD);
              }
              puts("\nError : Invalid VCD");
              puts("        The disc content array wasn\'t found\n");
              free(vcd_hdr);
              exit(1);
            }
          }
          else {
            if (!noBin) {
              fclose(fVCD);
            }
            puts("\nError : Invalid VCD");
            puts("        The disc type array wasn\'t found\n");
            free(vcd_hdr);
            exit(1);
          }
        }
        else {
          if (!noBin) {
            fclose(fVCD);
          }
          puts("\nError : Invalid VCD");
          puts("        Malformed disc image entry (sector count)\n");
          free(vcd_hdr);
          exit(1);
        }
      }
    }
  }
  else {
    usage(argv[0],2,0);
  }
}
