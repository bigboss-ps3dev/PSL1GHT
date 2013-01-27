/* Sample showing use of libgem. 
 * Only PlayStation Move is needed.
 * Author: bigboss Date: January 2013 
 * Minimun Firmware needed: 3.41
 * You will need console tty ouput(native or network) (NO GRAPHICS SHOWED ON YOUR TV!!!!!) to use this sample you can use "socat udp-recv:18194 stdout"  on your pc or mac to listen and show message from your Playstation 3
 * This sample is using PlayStation Move like you are using it in xmb to navigate on different menus
 * Use PlayStation Move with xmb before load the sample
 * move button on your PlayStation Move reset SENX to 12.0f and SENY to 6.0f
 * triangle button on your PlayStation Move add 1.0f to SENX
 * circle button on your PlayStation Move add -1.0f to SENX
 * cross button on your PlayStation Move add 1.0f to SENY
 * square button on your PlayStation Move add -1.0f to SENY
 * trigger button on your PlayStation Move to simulate pad RIGHT/LEFT/UP/DOWN movements
 * cross button on your DualShock Pad go out
 * load it with ps3load gemlight.self
 */

#include <ppu-lv2.h>

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>

#include <sysutil/video.h>
#include <rsx/gcm_sys.h>
#include <rsx/rsx.h>

#include <io/pad.h>
#include "rsxutil.h"
#include <sysmodule/sysmodule.h>
#include <sysutil/sysutil.h>
#include <sys/memory.h>

#define printf debugPrintf




// PAD
padInfo padinfo;
padData paddata;

// main bucle running
int running = 1;
int gem_flag = 0;
extern gcmContextData *context;
extern rsxBuffer buffers[MAX_BUFFERS];
extern int currentBuffer;



int readPad()
{
  int ret = 1;

  int i;

  ioPadGetInfo (&padinfo);
  for (i = 0; i < 6; i++) {	// 7 is our Move device
    if (padinfo.status[i]) {
      ioPadGetData (i, &paddata);

      if (paddata.BTN_CROSS) {

        ret = 0;		// To exit it will go to XMB
      }
    }

  }
  return ret;

}

void loadModules()
{
  int ret;

  printf("Loading modules\n");
  ret=sysModuleLoad(SYSMODULE_GEM);
  printf("load gem module return %X\n", ret);

}

void unLoadModules()
{
  int ret;

  printf("Unloading modules\n");

  ret =sysModuleUnload(SYSMODULE_GEM);
  printf("unload gem module return %X\n", ret);
  

}

static void eventHandler(u64 status, u64 param, void *userdata)
{
  static struct XMBEvent {
    int drawing;
    int opened;
    int closed;
    int exit;
  } xmb;

  printf("Received event %lX\n", status);
  if (status == SYSUTIL_EXIT_GAME) {
    xmb.exit = 1;
    running = 0;
  } else if (status == SYSUTIL_MENU_OPEN) {
    xmb.opened = 1;
    xmb.closed = 0;
  } else if (status == SYSUTIL_MENU_CLOSE) {
    xmb.opened = 0;
    xmb.closed = 1;
  } else if (status == SYSUTIL_DRAW_BEGIN) {
    /* We must start drawing, to avoid the app freezing */
    xmb.drawing = 1;
  } else if (status == SYSUTIL_DRAW_END) {
    xmb.drawing = 0;
  }
}

// Finalize stuff

void endGame()
{

  endGem();

  // sysMemContainerDestroy(container1);
  
  unLoadModules();

  sysUtilUnregisterCallback(SYSUTIL_EVENT_SLOT0);

  ioPadEnd();

}

int initGame ()
{
  int ret;

  netInitialize();  //if we have not printf native tty console support we are going to use my network printf friendly see debug.c to customize with your network configuration
  debugInit();
  printf("Sample showing use of libgem.\n"); 
  printf("Only PlayStation Move is needed.\n");
  printf("Author: bigboss Date: January 2013\n");
  printf("Minimun Firmware needed: 3.41\n");
  printf("You will need console tty ouput(native or network) to use this sample you can use socat udp-recv:18194 stdout  on your pc or mac to listen and show message from your Playstation 3\n");
  printf("This sample is using PlayStation Move like you are using it in xmb to navigate on different menus\n");
  printf("Use PlayStation Move with xmb before load the sample\n");
  printf("move button on your PlayStation Move reset SENX to 12.0f and SENY to 6.0f\n");
  printf("triangle button on your PlayStation Move add 1.0f to SENX\n");
  printf("circle button on your PlayStation Move add -1.0f to SENX\n");
  printf("cross button on your PlayStation Move add 1.0f to SENY\n");
  printf("square button on your PlayStation Move add -1.0f to SENY\n");
  printf("trigger button on your PlayStation Move to simulate pad RIGHT/LEFT/UP/DOWN movements\n");
  printf("cross button on your DualShock Pad go out");
  printf("load it with ps3load gemlight.self\n");
  // First load modules needed
  loadModules();
  // When we finalize this method is called
  atexit(endGame);
  // register callback
  sysUtilRegisterCallback(SYSUTIL_EVENT_SLOT0, eventHandler, NULL);
  // Init screen
  ret=startScreen();
  // Init pad we leave 7 for our move
  ioPadInit(6);
  // Init camera
  // printf("cameraInit() returned %d\n", cameraInit());
  initGem();
  return ret;

}

int main(s32 argc, const char *argv[])
{
  int ret;
  long frame=0;		// To keep track of how many frames we have rendered.
  running=initGame();
  // Ok, everything is setup. Now for the main loop.
  while(running) {
    // Check the pads. Press x on dualshock to go out.
    running=readPad();

    waitFlip();		// Wait for the last flip to finish, so we can
				// draw to the old buffer
    readGem();		 
    flip (context, buffers[currentBuffer].id);	// Flip buffer onto screen
    currentBuffer++;
    if (currentBuffer >= MAX_BUFFERS)
      currentBuffer = 0;
    sysUtilCheckCallback();
    
  }

  endScreen();

  return 0;
}
