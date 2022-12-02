#ifndef MMU_H
#define MMU_H

#include "cpu_utils.h"
#include "tlb.h"

int traducir_direccion_logica(int);//recibe una direccion logica y devuelve la direccion fisica fijandose si esta en la tlb antes de calcularla, si no esta ahi hace todas las validaciones para agregarla

int esta_en_memoria(uint32_t, uint32_t);//le pregunta a la memoria si un numero de pagina perteneciente a un segmento esta cargado o no

bool es_direccion_fisica_valida(uint32_t, uint32_t);//le pregunta a la memoria si una direccion fisica es valida o no

#endif