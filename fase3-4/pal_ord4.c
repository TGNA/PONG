#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <pthread.h>
#include "memoria.h"
#include "winsuport2.h"
#include "semafor.h"
#include "missatge.h"
#include <stdbool.h>

typedef struct{
  int ipo_pf, ipo_pc;       /* posicio del la paleta de l'ordinador */
  float po_pf;  /* pos. vertical de la paleta de l'ordinador, en valor real */
  float v_pal;      /* velocitat de la paleta del programa */
} Paleta;

int n_fil, n_col, l_pal, id_n_moviments, id_tecla, id_cont, retard, id_busties, numPaletes;
Paleta paleta;
int *p_win, *p_n_moviments, *p_tecla, *p_cont, *p_busties;

int id_sem_pantalla, id_sem_vglobals;

bool existeixCuaPaletes(int cua[], int paleta, int limit){
  int i=0;
  while(i<limit){
    if(cua[i] == paleta) return true;
    i++;
  }
  return false;
}

int main(int n_args, char *ll_args[]){
  int i, j,f_h, index, desplacament;
  char pos, mis[4], mis_aux[4];
  bool moure;

  n_fil = atoi(ll_args[1]);
  n_col = atoi(ll_args[2]);
  l_pal = atoi(ll_args[3]);

  paleta.ipo_pf = atoi(ll_args[4]);
  paleta.ipo_pc = atoi(ll_args[5]);
  paleta.po_pf = atof(ll_args[6]);
  paleta.v_pal = atof(ll_args[7]);

  p_win = map_mem(atoi(ll_args[8]));
  p_n_moviments = map_mem(atoi(ll_args[9]));
  p_tecla = map_mem(atoi(ll_args[10]));
  p_cont = map_mem(atoi(ll_args[11]));

  index = (atoi(ll_args[12]))-1;

  id_sem_pantalla = atoi(ll_args[13]);
  id_sem_vglobals = atoi(ll_args[14]);

  retard = atoi(ll_args[15]);

  p_busties = map_mem(atoi(ll_args[16]));

  numPaletes = atoi(ll_args[17]);

  win_set(p_win, n_fil, n_col);

  int cua_paletes[l_pal];

  do{
    win_retard(retard);
    if (p_busties[index] == -1)continue;
    receiveM(p_busties[index],mis);

    if(atoi(mis) == 0) desplacament = 0;
    else if(atoi(mis) == 1) desplacament = 1;
    else if(atoi(mis) == 2) desplacament = -1;

    if(desplacament != 0){
      waitS(id_sem_pantalla);
      moure=true;
      j=0;
      for (i = 0; i < l_pal; i++) {
        cua_paletes[i]=0;
      }
      for (i = 1; i <= l_pal; i++) {
        pos = win_quincar(paleta.ipo_pf+l_pal-i, paleta.ipo_pc+desplacament);
        if(pos != ' ' && pos != '+'){
          if(!existeixCuaPaletes(cua_paletes, (int)pos-48, i)){
            cua_paletes[j++]=(int)pos-48;
          }
          if(moure)moure=false;
        }
      }
      if(moure){
        if(paleta.ipo_pc+desplacament == (n_col-1)){
          for(i=1; i<=l_pal; i++)
            win_escricar(paleta.ipo_pf+l_pal-i,paleta.ipo_pc,' ',NO_INV);
          p_busties[index] = -1;
        }
        else{
          for(i=1; i<=l_pal; i++){
            win_escricar(paleta.ipo_pf+l_pal-i,paleta.ipo_pc,' ',NO_INV);
            win_escricar(paleta.ipo_pf+l_pal-i,paleta.ipo_pc+desplacament,ll_args[12][0],INVERS);
          }
          paleta.ipo_pc += desplacament;
        }
      }else{

        for ( i = 0; i<numPaletes; i++) {
          if(p_busties[i] == -1) continue;
          if(existeixCuaPaletes(cua_paletes, i+1, j)) sprintf(mis_aux, "%s", mis);
          else sprintf(mis_aux, "%i", 0);
          sendM(p_busties[i],mis_aux,4);
        }
      }
      signalS(id_sem_pantalla);
    }


    f_h = paleta.po_pf + paleta.v_pal;    /* posicio hipotetica de la paleta */
    if (f_h != paleta.ipo_pf){ /* si pos. hipotetica no coincideix amb pos. actual */
      if (paleta.v_pal > 0.0){     /* verificar moviment cap avall */
        waitS(id_sem_pantalla);
        if (win_quincar(f_h+l_pal-1,paleta.ipo_pc) == ' '){   /* si no hi ha obstacle */
          win_escricar(paleta.ipo_pf,paleta.ipo_pc,' ',NO_INV);     /*  esborra primer bloc*/
          paleta.po_pf += paleta.v_pal; paleta.ipo_pf = paleta.po_pf;   /* actualitza posicio */
          win_escricar(paleta.ipo_pf+l_pal-1,paleta.ipo_pc,ll_args[12][0],INVERS); /* impr. ultim bloc */
          signalS(id_sem_pantalla);
        }else{    /* si hi ha obstacle, canvia el sentit del moviment */
          signalS(id_sem_pantalla);
          paleta.v_pal = -paleta.v_pal;
        }
      }else{      /* verificar moviment cap amunt */
        waitS(id_sem_pantalla);
        if (win_quincar(f_h,paleta.ipo_pc) == ' '){        /* si no hi ha obstacle */
          win_escricar(paleta.ipo_pf+l_pal-1,paleta.ipo_pc,' ',NO_INV); /* esbo. ultim bloc */
          paleta.po_pf += paleta.v_pal; paleta.ipo_pf = paleta.po_pf;   /* actualitza posicio */
          win_escricar(paleta.ipo_pf,paleta.ipo_pc,ll_args[12][0],INVERS);  /* impr. primer bloc */
          signalS(id_sem_pantalla);
        }else{    /* si hi ha obstacle, canvia el sentit del moviment */
          signalS(id_sem_pantalla);
          paleta.v_pal = -paleta.v_pal;
        }
      }
    }else paleta.po_pf += paleta.v_pal;   /* actualitza posicio vertical real de la paleta */
    waitS(id_sem_vglobals);
    if(*p_n_moviments>0){
      *p_n_moviments=*p_n_moviments-1;
    }
    signalS(id_sem_vglobals);
  } while ((*p_tecla != TEC_RETURN) && (*p_cont==-1) && (*p_n_moviments>0));
  
  return 0;
}
