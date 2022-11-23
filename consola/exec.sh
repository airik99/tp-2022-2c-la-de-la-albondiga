#!/bin/bash
FILE=consola
make $FILE
if test -f "./$FILE"; then
    ./$FILE cfg/Consola.config cfg/pseudocodigo.txt 
fi