#ifndef TLB_H
#define TLB_H

#include "cpu_utils.h"

void agregar_a_tlb(t_traduccion*);//agrega una traduccion a la tlb en la posicion 0, desplazando las otras un espacio

void reemplazo_tlb(t_traduccion*);//reemplaza una traduccion de la tlb dependiendo del algoritmo (LRU o FIFO)

uint32_t buscar_el_maximo_instante_de_carga(t_list*);//busca el maximo instante de carga de una lista de traducciones (se usa en el algoritmo LRU)

void inicializar_tlb();//inicializa estructura tlb

void vaciar_tlb();//limpia la lista de la tlb

bool tlb_llena();//devuelve true si la tlb esta llena segun el archivo de config

uint32_t buscar_en_tlb(uint32_t, uint32_t ) ;//busca una traduccion en la tlb recibiendo como parametro una pagina y un segmento y la devuelve

bool esta_en_tlb(uint32_t, uint32_t);//devuelve si una pagina y un segmento estan en la tlb

bool es_algoritmo(char*);//hace strcmp entre lo que recibe y el algoritmo de la config

void actualizar_ultima_referencia(t_traduccion*);//se usa en buscar_en_tlb para que en el caso que el algoritmo sea LRU hacer eso actualizar el instante de carga

t_traduccion* actualizar_instante_de_carga(t_traduccion*);//se fija si la traduccion que recibo por parametro es la que busco se pone en 0, sino le suma 1 al instante de carga

bool es_la_traduccion_que_busco(t_traduccion*, t_traduccion*);//recibe dos traducciones y compara si son exactamente iguales

#endif