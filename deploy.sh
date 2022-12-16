#!/bin/bash
length=$(($#-1))
OPTIONS=${@:1:$length}
REPONAME="${!#}"
CWD=$PWD
echo -e "\n\nInstalling commons libraries...\n\n"
COMMONS="so-commons-library"
git clone "https://github.com/sisoputnfrba/${COMMONS}.git" $COMMONS
cd $COMMONS
sudo make uninstall
make all
sudo make install
cd $CWD
echo -e "\n\nHaciendo mkdirsXD...\n\n"
mkdir obj "$PWD/memoria/"
echo -e "\n\nAhi viene el efecto...\n\n"
make -C ./memoria
echo -e "\n\nListo maquina!\n\n"