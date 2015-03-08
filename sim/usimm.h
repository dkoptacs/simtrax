#ifndef USIMM_H_
#define USIMM_H_
int usimm_setup(char* config_filename, char* usimm_vi_file);
float getUsimmPower();
void printUsimmStats();
void usimmClock();
bool usimmIsBusy();
#endif
