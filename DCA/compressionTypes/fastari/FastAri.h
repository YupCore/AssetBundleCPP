﻿#pragma once
/*  FastAri 0.3 - A Fast Block-Based Bitwise Arithmetic Compressor
Copyright (C) 2013  David Catt

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program. If not, see http://www.gnu.org/licenses/
Modified by YupCore.
                                                                         
                                                                                            
                                                                                            _    _  _____   _____    __  
                                                                                     /\    | |  | ||  __ \ |_   _|  _\ \ 
                                                                                    /  \   | |__| || |__) |  | |   (_)| |
                                                                                   / /\ \  |  __  ||  _  /   | |      | |
                                                                                  / ____ \ | |  | || | \ \  _| |_   _ | |
                                                                                 /_/    \_\|_|  |_||_|  \_\|_____| (_)| |
                                                                                                                     /_/ 

                                                                :?:
                                                               .~^?
                                                              .~..7~                                                                        .~
                                                              ~: .:?~                                                                    .^~7~
                                                             ^~....~J:                                                                :^~~::^
                                                            .~:.....!J.                                                          .:^~~^:.:..
                                                            ^~......:77                                                      .:~~~~^:...:::
                                                           .~^.......!7!                                                 :^~~~~^::.....:^:
                                                           ^~:.......:77^                                            .^~~~^^~^:.......:~^
                                                           ~~.........^??^                                       .:~~~~^^~!~:........:!^
                                                           ~^.......::.^7?:                                   .^~~~^^^^~!~:..::.....^!:
                                                          .~:.:::::.:::.~7?:                               .^~~~^~~!!!!~^^~~:::....^!.
                                                          :!:.:::::^:::::!!?!  :^~~~!~~~~77?77!7??!~~^. :~?!~~!!7!^::...^^::::::.:~!.
                                                          ^~.:::::~!^:::::!~7J?!!!777!!77J7!!!!777?7!!77777~~!~^:.....~!:::::::.^!7
                                                          ^~.::::^^.7~:::^^7~!J?^!!!7JJ?77~~!!!~~~!77777!:::~^^^:::^!!!^:!:::::^!^
                                                          ^!.::::^^.:!!~^~7!~~!?J!!!!!~~!7!!!!!~~~^^^^~~~~~!7!!!~!~:....~^::::~!^
                                                          ~!.::::~~..:~!!!!?7!!!!~^^^^~~^~!!!!!!!!!~^^^~~~~~~!!!!^::^^~7!^::^77777.
                                                          ~!.^~^^~!:::::^!7!!!~~~~~~^^~!!~~~!!!~!!~~~~^^~~~!!~~~!!!~~~^:^:..:::~~!!
                                                          ~7.:^!~^:.:~~!7!~!!~~!~~^^~~^!!!!~~~~!~~7~~!!~~~~~!!!~^~!!!^^:.::^^^7J!!^
                                                       :^:??^^~^77!^:~?!~~!~~!~~^^^~!^^^!!!!~^^^~~~!~~!!~^^^^~!!~~^^~7!~~:::^^~!7J.
                                                     ^~~!!!. ..:^^!7!7~^~!~!~^^^^^~~~^^^^!7!!~^^^~^^!^~~7~^^^^~!!!~^^^!?: :^~~~~^~7
                                                    .J::!~......:^^!?^^!!~!~^^^^^^^!~^^^^^!7!!~^^^^^^~^^~7~^^^^^!7!~^^^~!!!^:.!?^^?:..
                                                     ^^^7!~^^::.:^^!J^~!!~^^^^^^^^~!^^^^^^^77!!^^^^^^^^^^~?~^^^^^~!7~^^^!7^::::!?!77!~~~:
                                                  ..:^?!7::^~??~!!!7J~!!~^^^^^^^^^~!^^^^^^^^?!!^^^^^^^^^^^!?^^^^^^~!7~^^::~?JJJ7!^^!!^:.
                                              .:::^~!7??!7^:7Y7^~!77~!!~^^^^^^^^^^~~^^^^^^^^~7!^^^^^^^^^^^^?!^^^^^^~!!~:...~5YYJ7!~:^:
                                            :::^~~~!??J^!7YYY?!?!~^^~!~::^^^^^^^^^~~^^^^^^^^^!!^^^^^^^^^^^^~J~^^^^^^~~!:.:::!Y!7?!~~~^.
                                           ~~~~~~^!!!J^.7!7J?J7?:..:~7:.:^^^^::^^^^~^^^^^^^^^^7~^^^^^^^^^^^^?!^::.::^~~!:::::7?~~~~~~!~
                                           .~~~~~~~~?~.:!~!?7JJ~:::^!!:::^::...::^~!^..::.::^^7!^.:~^::::..:!7^:...::^^~~^^^^^7!~~~:.
                                             .^~~~~??..^~~~!77?^^^^^7~::^^::....::~7~...::..::~7:..~:......:!?~::::::^!^!^^^^^^7!^
                                                .~7~^:.!~~~~!!7~^^^^!^^^~:::::::::~??^:.:^:.::^?^::!::::::::?J^^^^^^^^~!!^^^^^^^!
                                                    ~?^7~!!!!!!~^^^^~^^^!^^^:::::^!J?7^::^^^:^:7~:~!^^^^^^:^?!7~^^^^^^^7?^^^^^^^^~.
                                                   ~?Y777!!!!!7^^^^^~^^~!^^^^^^^^~?!.J!^^^^^^^:!~^7~^^^^^^^!~.:?7^^^^^^^?J~~~~^^^^~:
                                                  !Y!?~!?!!~^~7^^^^^^^^!!^^^^^^~~7J..:J!^^^^^^:~!!7^^^^^^^!~   .7Y7~^^^^^?Y7!~~~^^^~^
                                                :7J!!7~7?~^^^!?~^^^^^~~7~^^^^^~~!?:   .?!^^^~^^~!7!^^^^^^~^..:::^!YJ7!~^^^!?77!!~~^^^~:
                                              .^^.~^:7~J7^^^^!7~^^^^^~!!^^^^^~~!7.     .!7~~~!^~77^^^^^~7~^^:..   :!7777!~^^!!!JJ!7?7!~~~:.
                                           .^:   :~ :7~J!^^^^!7^^^^^~~!~^^^^^~!7.   .....^77^!!?Y!^^^^~7:. ..:^!7!7~J7^~!77!!7!?J7..^~^:....
                                         .^:    :^  ^!!J^^^^~!!^^^^^~!~^^^^^~7?^^^^^^^^^:::~!~?5Y^^~7!^..^JPGBBYJ?Y!^57:^^~!!??7?Y~
                                      .^~:     ::   ~~77^^^^~7^^^^^~7?^^^^^^7~. ....::...   .:~Y!~~7~. .^GGGG!!. ^7.:7Y~:::^^~77^:?:
                                     ..       ^:    ~~J~^^^~?!^^^~~7Y^^^^^~7!^7JJ5PPBGGPJ7^.  ~7~!:.   .77!!~^~:!7:.~^~?...::!^77.:!^
                                            .~:     !~J^^^~77^^^~!?Y^:^^^!Y^~?P5!~5JYPJ~~.:^. .  ..    ...:^^^:::...!^^?:.:::J^7~!.:~~
                                           ^~^      !!!^^~?7^^~!!7?^:^~!!~7^..:7!^!!~~!7^.. .         ............:!7^^J^::^!7~7^^!..^^
                                          ~!!      .!7~!7?!^!77!!!~!?77~::!7:...^^^~^^::....        .............^^!!~?!^~!777!^~~^!.
                                         ^!!:      :??!777!7!!7?777G7^^^..^!!~:...............     ..............:~J?!!~!?J!7~^^^!~^~:
                                        .?!!       ^J~~!~~~^~~~~^^JY~:^^:..~~^~^^^:::..........     ..::.........:!J~~^^^~57^^::^^!^..:.
                                         ^?!       7?^^^~!:..:^^^7Y!!~^::::.^!!^^^:::..........   .. .:...........!J^^^^^~?J7^..::^7^..:^.
                                          .^       ?^^^!!::.:~^^!??^^7?7~^::::^^~!7!^..........   .. ............7JJ!^^^~~???7::::^~?^:::~^.
                                                  .!^~!7^^:^~^.!7?~:!7~^^?J~^^^^::^^^.......            ........~?~!J~^^:~7?7?7^^^^~?7^^^^~!~.
                                                  !~~7!^^~~~^:!7!7^77^:.:?7?7~^^^~^:.......   .........:::... :77~^~?!:.:!!7~7!7~^^~!J!~~~~~!?~.
                                                 :!~7!^~!!~^^7?~!!7!^::.!?~~JP?^.. .......    .:.::^~^^:... .~7!!7^^!7:.:!~?^!~~7~^~!JJ~~~~!!!??:
                                                 7!7!!!!!~~~??~~??~^^^:^J~^~J5Y5Y?~:.            ........ .^??!~^?~^~7^:^7~7~~?:^7~!!JY~~~~!!!7Y7~.
                                                !7?77!!~~~!??^^?7~^^^^^!7^^~JYJJY555Y7^..           .... .!~.:^~~7?~!7^^!!~7~!77:!7~!YJ!~~~~~!7!:7!:
                                               ~Y?7!!!~^~!?7::77^^^^^^^?~!~~5JJJJJJY5PP5Y?~:.          .~Y^.....:::::!~^?!~??77J!^7~7??~~~~~~~?^ ^77
                                              ~J?!!!~^^~7?^.:7!^^::^::~?^7~?5YJJJJYYYY555PPP5J?!^.....!Y5J::...:::::.~^~7~^7~:~77^7!!?!^~~~~~~?. !77
                                           .^?J7!!~^^^~?!..^7!^~^.::.:J!^?77JJJYYYYYYYYYY55555PPPP5YY5P5J?.....::::::~^7~~^7!:^!!!J!~?~7!~~~~7^.!!7:
                                         .~!J57!^^^^^!?~..^?~^~~::..:^5!77^.:~!7?JYYYYYYYYYYY555PPPPPPP5?7~:....::::^~7!~^^7!:~!77!~77J7~~~~7?^!~^.
                                       .^!7777^::~^~??~::^?~~!!~:^:::!YY^.....:^^^^7JYYYYYYJYYYY5555555?7:?7....::::!?7~~^^7~^~!?::7JJ7~~~!!J?^.
                                      :~~7!^!^:::^!J7~~^^7~~57!~:^^^:?7?^.......::::^!7?JJJYYYY5555555J?^^7^......:~?!~~~^^7~~!!::::?7!!~!?7~!~    :.
                                    .!~~?^.!^:::^7?7~~~^!7^Y??!~^^^^^?!^7:..........:^~^^~!7?J5J?JJ??JJ7!7?!~....:77!~~~^^~?7~^::::.~777??!~~~7~~7J5!
                                   :7~~?!:!~^^^!7!7~~!7~?~7J~7!~^^^^~?!^~7................:::^~!7J??JJ~77!JJ!...^77~~77777?!^:.::::.:?JJ7~^~7JY5YYJ?7
                                  .!~~?7^!~^~7J?~?!~~?7~?~J!~:!!~^^^^?!^^~!.....:^..............:^^!????7?7~..:~!7~^^^~~~~7:..::....:J7??!7YYYYYYJJ??
                                  :~~7?J7^~!?J?7!J~~~JJ!?!7:...!!~^^^7!^^^~7^....~7:  ..............^~~::7?7~^!!7~^^^^^^^7^.....::..:?7?Y5YYYYYYYY7J?
                                . ~~!7JJ^!7??77~~5!!77^::77:..::!7!^^~7^::^~!~. . ^!^           .......  .^~!~77^^^^^^^^~!....  ^:...7Y55YYYYYYY5J7Y7
                                . .!!77~777J7!~^~Y7^.. . .?7^....:!7~^7!^:.:^~!^::^:!......           .   ^~^^7.^^^^^^^^7:.     .^^:.J55YYYYYYY5?!YYY~~^^!?:
                                .. .7!77!!J?!~~7^.   ...  .^!!^:...:!?!7!~^:^^~!!~~!7:......            .~!. !~.:^^^^^:~!.        .^:?5YYYYYY5J7!JYJY::~??Y:
                                   :!?7!!!J!~^7^   .        .:^~~~^^:::^7J77777!!77?!~:.               ..?!  !^::^^:.::~~.   . :.   .^Y55YY5Y!!?JJJYY7?????                     .....
                                  .7J!~!!7J~^~~ ...                     .~??7777!~~~!~~~~~^..          .:?~^.!~^^^^^:::^!:  ..:~   :. .~J55?.~J?YYYY?????7^                       .:^~~^:
                                  !J!!!!!7J~^?....                        .~!!~~~~^^^~!!~~~~!~^:..      .!~^^~!^~~^^^^^^~!~~~!!..:~:    .7~:7JY5Y?77??777?7.                          :~~~~.
                                 :Y7!!~~!!J~~? ..                           .:^~~!!!~~~~!!~..:^~!~^^.    .:.  .~~!!!~~~~~~~!!7!7?^.    .^:~Y55J7!??777777?7!:                          .~~~7.
                                 ??!!~^^~~?77!                                ..::^~~!!!~~!!!:::^~^.^~.         .^!!!!!!!!!!7!~^..  . :^:755?!!??77!!!77!?7~~~:                         :~~7:
                                .5!~~~^:^~~?J^                                    ........::~!~^^^!^ ^?^          .~!!777?7^:..:^:::.^:^Y5?~!??!!!!777777??7!::!~.                      .~~7.
                                7!7^^^^:.:~~Y~                                              ..~!^^~7:!7~            .:^~~~~~~~!7^:..^.~5?~~77!~~~~!!!!!7!77!??~^!?~.                    .~~.
                               :7.:!:.:^:.:^!?.                                     .         :J~^~?^:.      .....     .........  .:.!5!^~!~^^^^~~~~~!!!7??~~!!!!!777~:.               .!~.
                               ~!^:Y!:::77^:^7~                        ..           .:...   .:~77!7:         .......           . .^ 7J^^~^:^^^^~~!!!!7??!~??!~^~~!7~!?7!!~^:.........:~!:
                               .7!J!77!~^!?7!~?:.                       .::..        .~!^^:^~!7J7~.             ...::.......... :~:77:~~^^^^^~~~~!!!?7!~^^~?!77~^~5^.:^~!777!~^^:^^~!~.
                                :?!77?Y?7!!7JY55..                 ...::::~~~~~~~^~~!~~7777????!~^:::........      ..:^:.......:~:7!:!~^^:^~~~~~~^..!7!~^~~!~^^::??7....:^~~!!7!~:.
                               .!!!?JYJ?JYYYYYY5J..       ....:^~77?JJYYYYYYYYYYYJJJJJJJJJYJJJJJ?JJJJ???777~~~^:::.....^^.....:^:7^^!~^^::^~~~^.   .~?!7~^^::::^77~JY?!~^^^^^^^^~~^^^:.
     ..                      .~7~!?JJ7?5555YY55YPJ....::^~!7?JY555YJJ?7!~~~~~~~~~~~~~~~~~~^^^^^^^^^^~~~~!!7?JJJYJJ?7!~^^~~:..:^.7^^!^^^:^~^~~:     .^J!^::::::~7!~~77JYJ77!!!!!!~~~~~~!~~^.
      :^                   :~7!!7JY?!YYJ????????7?!!!7JYY5YYJ?77!!~~~~~~!!~~^~~~~~~^^::::::::^^^^^^~~~~~~~~~~~!!!7?JJYYJ??7~^^.7~^!^^^:^~~~:      .^^:.:::::^!7~~~^!!~!?J?!!!!!7?Y?77^^^~~!!~:.
       .!!^.           .:^!7!!7???!~~!!JYJJJJJJJYYYYYYJ?77!!!~~~~~~~~~~^^:^^~^^::::^^^^^^^^^^^^^^::::::^::^^^~~~~!!!!!7JJYJYY:~!^~^^^^:^~^.     .^^:...::^^~~^^^~^~!7^^^~7?7!~~~~75J?~     .~77!^.
         .!?7!~~^^^~~~~!77777!7?7~~~!!7!^!JYYYYYJ??7!!!!!~~~~~~^^^^^:::^^^^^::^^~~~~~~~~~~~^^^^^^^^^^^^^^^^^^^^^^^^~~~~!7?JJ!!!~~^^^^^^!:      :^:....:^^~~^^^^^~^~~7^^^^^~77!!~^^~J777^     .~!!7!.
            :^~!!!!!7777?77~~!?7~~!77~::^~7J?77!!!!!!!!!!!!~~~~~~^^^^^^^:^^~~~~~~~~~~~^^^::.....          .....:^^^~~~~~~~!7^?~!^^^^^~~.     .^^..::^^~~^^^^^^^^^^~~7^^^~~^^!77!~^^^7!!77.     ^7~!!~.
                  .^!!7?7!~~!??~~?Y?7??J??77777777!!!!!~~~~~~~~~~~~^^:^^~~~!!!!!~^...                               ...:^^~^!!!^^^^~!^      ^~^^~!7!!^^^^^^^^^^^^~~~?~^^^~!~^777!~:.^?~~!?:     ?!~~~?!
               .^~~^!77!~^^~7Y!~~~7?J?77777777777!!!!!!!!!!~~~~~~^^^~!!~~~^:.^:....                                       ~^?!!^^^~!...:^~~!?7?77~~^^^^^~^~~~^^~~~~~??!~~^~!!~777!:..?!~~!?:    !7~!~~J?
           .:^^^^:^77!~~^^^!JJ~~^~~~!7?7!777777!!!!!!!!!!!!!!~~~~~~~^::...   .!.....                                     .~^??~^^!!::~^^^~~~7J7~~~~^^^^~^~~~~^~~~~~!!!7?7!~~!!!?!7~::77^~~~?:   !~~!7^77
        .:^^..:.:!7!~~^~7~~???~~^~~^~!7?7!77777!!!!!!!!!!!!!!!~~^:....       .^^.......                                ..~~!Y?~~7!:7J!~^::::.~??!!7!~!~~~~~~~~~~~!~~~7~~7?77!!!?!7!^^7?~^~~!J. :7^~!7~7~
      :~~^..:. ^?!~~~~~^~?!~77~~^~~~!!77??7!!!!!!!!!!!!!!!~^^:.....          ..~:.........                     ....::::::~~!7!~~!~!77JJ7~::^^~!JJ7~~^^!7~~~~~!!!!!~!!!J!^~7??77J7~!~^77~:^~~!J:?~~!!7!Y~
   .^!!~^:^^  !?!~~~~^^^^!7!?7!~^~~~?!7~~7??777?77?777?J........      ..........~:...............  ..   ....::::^::::::::......:^~7?777?Y?: .:^^~7!:::~!77!!!~!!!!!7!77J7~~~!77Y7^!^~J7^.^^^~JY~^~!!??^!
  ~!~~~~^~:  !?!!~~~~::^~^775?!~^~~!?!7~^^~!!!JJ~~?~^~??...... ..................^^...................:^^^^^:::::.........::~!7777!!!77~7J. ..:^^~?7::~77!!!!!!!!!!!77!7JJ7!!!7J!~~~7J7^.:::~?~^^~!??. !
 :!~~~~~77   ?!!~~!7~::^~~7??J!!^^~!77?~~^^^^7?::7~^^77!........  ...      .......:^:.............:~^^:::::::::::::...::^~7?7!~~~~~!~^:..7!~^^^~77??~!??!!!!!!!!!777777777??7?!J~~~!~J7^:::!?~^~^!?~  ^!
 .7~~~~!J?. .J!!~~7J!^:^~~?!77?7~^~!!7?7~~~~7!.^J~:^~!~!^.....^^:.                  .::...........:?J?77!~^^^^::::::^^~~~~~^^^~~!~7~^^::::~^^~~~YJ!:..:^!?!!!!!!!!!777777?!7J~77^!7~~J!^^~7?~~~~!~.  ^?^
  .~!~~!??7..J!~~~~!?!^~!!~~7~7J7^~7!~7?~^~!^^!J~:~~~~~!7!~....::..:...........       .::^^.........^!?????????7!!~~~~~~~~~^^^~~~~~!!7~::^~77~^^!J?~:::::!JJ????7~~!!!!!~?77~~7^^~:^7!^^~7!~!!!7?  ...
    .~!!??7!~7?~~~~.7?77~~~7J^~?J!^!7~~~7!!!!7?~^!!!!~!!!7Y?^::.::.  ..::...  ...........:75Y?!~^::...^!7!~!~~~~~~~~~~~~~~~~~~~~~~~!!!!!7Y55~...:!55?~^7J?YP55JJ77!~~~^~!??!!?!~^^~77!!7!!7?~  !7
       :^!!!!!!!~!!~~~~^~~!JY?~~JY7!7~!77??77!~!77~?!!!7!JJ?^::.:::..    ......    .....:..^5BGGPP55YYYY55J!~!!!!~!!~!!~~~!!!!!~!~!!!!!!Y55?:.^::!PP55Y?Y5555555Y??7!7????J?7~~~!7?77??7!!?7   !~
*/

#include <stdlib.h>
#include <memory>
#define ORDER 17
#define ADAPT 4
const int mask = (1 << ORDER) - 1;
class FastAri
{
public:
	static int fa_compress(const unsigned char* ibuf, unsigned char* obuf, size_t ilen, size_t* olen);
	static int fa_decompress(const unsigned char* ibuf, unsigned char* obuf, size_t ilen, size_t olen);
};
