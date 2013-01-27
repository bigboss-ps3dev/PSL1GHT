#include <stdio.h>
#include <io/move.h>
#include <sys/memory.h>
#include <ppu-types.h>
#include <ppu-types.h>
#include <spurs/spurs.h>
#include <sys/spu.h>
#include <sys/thread.h>
#include <sys/systime.h>
#include <math.h>
#include "../include/rsxutil.h"
#define printf debugPrintf

#define SPURS_PREFIX_NAME "gemlight"

gemAttribute gem_attr;
gemInfo gem_info;
gemState gem_state;
gemInertialState gem_inertial_state;
u16 oldGemPad = 0;
u16 newGemPad = 0;
u16 newGemAnalogT = 0;

float gx,gy;
float SENX=12.0f;
float SENY=6.0f;


Spurs *spurs ATTRIBUTE_PRXPTR;
void *gem_memory ATTRIBUTE_PRXPTR;





static inline float vec_array (vec_float4 vec, unsigned int idx)
{
  union {
    vec_float4 vec;
    float array[4];
  } v;

  v.vec = vec;

  if (idx > 3)
    return -1;
  return v.array[idx];
}
int processGem(int t)
{

  int ret;

  switch (t) {

    case 0:

      ret = gemUpdateStart(0, 0);

      if (ret != 0) {
	//	printf("Return from gemUpdateStart %X\n", ret);
      }
      break;
    case 2:

      ret = gemUpdateFinish();
      if (ret != 0) {
	//	printf("Return from gemUpdateFinish %X\n", ret);
      }
      break;
    default:
      ret = -1;
      break;
  }
  return ret;
}

int initSpurs ()
{
  int ret;
  int i;
  sys_ppu_thread_t ppu_thread_id;
  int ppu_prio;
  unsigned int nthread;

  ret = sysSpuInitialize(6, 0); //you can choose how many spu to use
  printf("sysSpuInitialize return %d\n", ret);

  ret = sysThreadGetId(&ppu_thread_id);
  printf("sysThreadGetId return %d ppu_thread_id %x\n", ret, ppu_thread_id);

  ret = sysThreadGetPriority(ppu_thread_id, &ppu_prio);
  printf("sysThreadGetPriority return %d ppu_prio %d\n", ret, ppu_prio);

  /* initialize spurs */
  printf("Initializing spurs\n");
  spurs = (void *) memalign(SPURS_ALIGN, sizeof (Spurs));
  printf("Initializing spurs attribute\n");
  SpursAttribute attributeSpurs;

  ret = spursAttributeInitialize(&attributeSpurs, 5, 250, ppu_prio - 1, true);
  if (ret) {
    printf ("error : spursAttributeInitialize failed  %x\n", ret);
    return ret;
  }

  printf ("Setting name prefix\n");
  ret = spursAttributeSetNamePrefix (&attributeSpurs, SPURS_PREFIX_NAME,strlen (SPURS_PREFIX_NAME));
  if (ret) {
    printf("error : spursAttributeInitialize failed %x\n", ret);
    return ret;
  }

  printf ("Initializing with attribute\n");
  ret = spursInitializeWithAttribute (spurs, &attributeSpurs);
  if (ret) {
    printf ("error: spursInitializeWithAttribute failed  %x\n", ret);
    return ret;
  }

  ret = spursGetNumSpuThread (spurs, &nthread);
  if (ret) {
    printf("error: spursGetNumSpuThread failed %x\n", ret);
  }

  sys_spu_thread_t *threads =(sys_spu_thread_t *) malloc (sizeof (sys_spu_thread_t) * nthread);

  ret = spursGetSpuThreadId (spurs, threads, &nthread);
  if (ret) {
    printf ("error: spursGetSpuThreadId failed %x\n", ret);
  }

  printf("SPURS %d spu threads availables\n", nthread);
  for (i = 0; i < nthread; i++) {
    printf ("SPU Number:%d\tSPU Thread ID:%x\n", i, threads[i]);
  }
  printf("\n");

  printf("checking SpursInfo\n");
  SpursInfo info;

  ret = spursGetInfo(spurs, &info);
  if (ret) {
    printf("error: spursGetInfo failed %x\n", ret);
  }
  printf("SpursInfo: \n");
  printf("nSpus=%d \n", info.nSpus);
  printf("spuGroupPriority=%d \n", info.spuGroupPriority);
  printf("ppuThreadPriority=%d \n", info.ppuThreadPriority);
  printf("exitIfNoWork=%d \n", info.exitIfNoWork);
  printf("namePrefix=%s \n", info.namePrefix);
  for (i = 0; i < info.nSpus; i++) {
    printf("SPU Number:%d\tSPU Thread ID:%x\n", i, info.spuThreads[i]);
  }
  printf("SPURS initialized correctly!!!\n");

}

int endSpurs ()
{
  spursFinalize(spurs);
  return 0;
}

int endGem ()
{
  endSpurs();
  gemEnd();
  return 0;
}

static inline void initAttributeGem (gemAttribute * attribute, u32 max_connect, void *memory_ptr,Spurs * spurs, const u8 spu_priorities[8])
{
  int i;

  attribute->version = 2;
  attribute->max = max_connect;
  attribute->spurs = spurs;
  attribute->memory = memory_ptr;
  for (i = 0; i < 8; ++i)
    attribute->spu_priorities[i] = spu_priorities[i];
}

int initGem ()
{
  int ret;
  int i;

  initSpurs ();

  ret = gemGetMemorySize (1);
  printf("return from GemGetMemorySize %X size in bytes needed for move device to init libgem\n",ret);
  gem_memory = malloc (ret);

  printf("preparing GemAttribute structure with sprus and memory stuff is very important align correctly spurs structure \n");
  u8 gem_spu_priorities[8] = { 1, 0, 0, 0, 0, 0, 0, 0 };	
  gemAttribute gem_attr;

  initAttributeGem (&gem_attr, 1, NULL, spurs, gem_spu_priorities);

  printf("calling GemInit with GemAttribute structure version=%d max_connect=%d spurs=%X memory_ptr=%X  \n",gem_attr.version, gem_attr.max, gem_attr.spurs, gem_attr.memory);
  ret = gemInit(&gem_attr);
  printf("return from GemInit %X \n", ret);
  //ret = gemReset(0);
  //printf("GemReset return %X \n", ret);
  return ret;

}

void readGemPad (int num_gem)
{
  int ret;
  //ret = gemGetState (0, 0, 0, &gem_state);
  ret = gemGetInertialState (num_gem, 0, 0, &gem_inertial_state);
  newGemPad = gem_inertial_state.pad.buttons & (~oldGemPad);
  newGemAnalogT = gem_inertial_state.pad.ANA_T;
  oldGemPad = gem_inertial_state.pad.buttons;

  

}

void readGemAccPosition (int num_gem)
{
  vec_float4 position;

  gemGetAccelerometerPositionInDevice (num_gem, &position);

  printf(" accelerometer device coordinates [%f,%f,%f,%f]\n",
      vec_array (position, 0), vec_array (position, 1), vec_array (position, 2),
      vec_array (position, 3));

}

void readGem ()
{

  
  processGem (0);
  processGem (2);
  
  readGemPad (0);		// This will read buttons from Move
  switch (newGemPad) {
    case 1:
      printf("Select pressed \n");
      break;
    case 2:
      printf("T pressed value %d\n", newGemAnalogT);
	  break;
    case 4:
      printf("Move pressed \n");
	  SENX=12.0f;
	  SENY=6.0f;
	  printf("SENX set to %f SENY set to %f \n",SENX,SENY);
      break;
    case 8:
      printf ("Start pressed \n");
      break;
    case 16:
      printf ("Triangle pressed \n");
		SENX=SENX+1.0f;
		printf("SENX set to %f\n",SENX);
      break;
    case 32:
      printf ("Circle pressed \n");
	  	SENX=SENX-1.0f;
		printf("SENX set to %f\n",SENX);
      break;
    case 64:
      printf ("Cross pressed \n");
		SENY=SENY+1.0f;
		printf("SENY set to %f\n",SENY);
      break;
    case 128:
      printf ("Square pressed \n");
		SENY=SENY-1.0f;
		printf("SENY set to %f\n",SENY);
      break;
    default:
      break;
  }
	if(newGemAnalogT) //if we leave pressed trigger  we are going to simulate RIGHT/LEFT/UP/DOWN pad 
	{
	
			

		gx+=-vec_array(gem_inertial_state.gyro,1);
		gy+=vec_array(gem_inertial_state.gyro,0);
		if(fabs(gx)>fabs(gy)) //if we have more inertial on x
		{
			
			if(gx>SENX) //you can change this value to adapt sensibility on left/right 
			{
				printf("RIGHT \n");
				gx=gx-SENX;
				gy=0.0f;
				
			}
			else
			{
				if(gx<-SENX) //you can change this value to adapt sensibility on left/right 
				{
					printf("LEFT \n");
					gx=gx+SENX;
					gy=0.0f;
				}
					
			}
		//	printf("gx %f \n",gx);
		
			
		}
		else //if we have more inertial on y
		{
				if(gy>SENY)//you can change this value to adapt sensibility on up/down 
				{
					printf("UP\n");
					gy=gy-SENY;
					gx=0.0f;
				}
				else
				{
					if(gy<-SENY)//you can change this value to adapt sensivity on up/down
					{
						printf("DOWN\n");
						gy=gy+SENY;
						gx=0.0f;
					}
				}
		//	printf("gy %f \n",gy);
		}
			            
	}
	else //if no trigger reset inertial 
	{
		gx=0.0f;
		gy=0.0f;
	}
	

}


