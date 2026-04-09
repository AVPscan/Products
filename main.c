/*
 * Copyright (C) 2026 Поздняков Алексей Васильевич
 * E-mail: avp70ru@mail.ru
 * 
 * Данная программа является свободным программным обеспечением: вы можете 
 * распространять ее и/или изменять согласно условиям Стандартной общественной 
 * лицензии GNU (GPLv3).
 */
 
#include "sys.h"

void help() {
    os_printf(Cnn "Created by " Cna "Alexey Pozdnyakov" Cnn " in " Cna "01.2026" Cnn 
           " version " Cna "1.41" Cnn ", email " Cna "avp70ru@mail.ru" Cnn 
           " github " Cna "https://github.com/AVPscan" Cnn "\n"); }

int main(int argc, char *argv[]) {
    if (argc > 1) { if (strcmp(argv[1], "-?") == 0 || strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "-help") == 0) help();
                    return 0; }
    Run(); return 0; }
