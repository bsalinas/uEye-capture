#include "camera.h"

static void chk(int st, Camera* cam) {
  if (st != IS_SUCCESS) {
	char* pErr = 0;
	is_GetError(cam->hCam, &st, &pErr);
    fprintf(stderr, "API Error: %d(%s)\n", st,pErr);
    exit(st);
  }
}

int cam_connect(Camera *camera) {
  fprintf(stderr, "Connecting...");
  HIDS hCam = 0;

  chk(is_InitCamera(&hCam, NULL),camera);

  fprintf(stderr, "connected to camera: %d\n", hCam);
  camera->hCam = hCam;
  
/*  UINT count;
UINT bytesNeeded = sizeof(IMAGE_FORMAT_LIST);
int nRet = is_ImageFormat(hCam, IMGFRMT_CMD_GET_NUM_ENTRIES, &count, 4);
bytesNeeded += (count - 1) * sizeof(IMAGE_FORMAT_INFO);
void* ptr = malloc(bytesNeeded);
 
// Create and fill list
IMAGE_FORMAT_LIST* pformatList = (IMAGE_FORMAT_LIST*) ptr;
pformatList->nSizeOfListEntry = sizeof(IMAGE_FORMAT_INFO);
pformatList->nNumListElements = count;
nRet = is_ImageFormat(hCam, IMGFRMT_CMD_GET_LIST, pformatList, bytesNeeded);
 
// Prepare for creating image buffers
char* pMem = NULL;
int memID = 0;
 
// Set each format and then capture an image
IMAGE_FORMAT_INFO formatInfo;
int i;
for (i = 0; i < count; i++)
{
    formatInfo = pformatList->FormatInfo[i];
    fprintf(stderr,"%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%s\n",
      formatInfo.nFormatID,
      formatInfo.nWidth,
      formatInfo.nHeight,
      formatInfo.nX0,
      formatInfo.nY0,
      formatInfo.nSupportedCaptureModes,
      formatInfo.nBinningMode,
      formatInfo.nSubsamplingMode,
      formatInfo.strFormatName
      );

    int width = formatInfo.nWidth;
    int height = formatInfo.nHeight;
 
    // Allocate image mem for current format, set format
    nRet = is_AllocImageMem(hCam, width, height, 24, &pMem, &memID);
    nRet = is_SetImageMem(hCam, pMem, memID);
    nRet = is_ImageFormat(hCam, IMGFRMT_CMD_SET_FORMAT, &formatInfo.nFormatID, 4);
 
    // Capture image
    nRet = is_FreezeVideo(hCam, IS_WAIT);
}

*/

  SENSORINFO sensinfo;
  chk(is_GetSensorInfo(hCam, &sensinfo),camera);

  fprintf(stderr, "\tSensor: %s\n"
                  "\tResolution: %ux%u\n"
                  "\tPixel Size: %.2f\n",
                  sensinfo.strSensorName,
                  sensinfo.nMaxWidth,
                  sensinfo.nMaxHeight,
                  sensinfo.wPixelSize/100.0);

  chk(is_GetSensorInfo(hCam, &sensinfo),camera);

  camera->width = sensinfo.nMaxWidth;
  camera->height = sensinfo.nMaxHeight;

  UINT nPixelClock = 9;
  // Get current pixel clock
  chk(is_PixelClock(hCam, IS_PIXELCLOCK_CMD_SET, (void*)&nPixelClock, sizeof(nPixelClock)),camera);
  fprintf(stderr, "Setting color mode\n");
  // use 8 bits per channel, with 3 channels. Need BGR order for correct
  // colour display when saving to BMP
  chk(is_SetColorMode(hCam, IS_CM_JPEG),camera);//IS_CM_BGR8_PACKED));

  // set a low exposure
  double exposure_ms = 1000;
  chk(is_Exposure(
        hCam,
        IS_EXPOSURE_CMD_SET_EXPOSURE,
        &exposure_ms,
        sizeof(exposure_ms)),camera);
int  formatId = 4;
chk(is_ImageFormat(hCam,IMGFRMT_CMD_SET_FORMAT,&formatId,4),camera);

  fprintf(stderr, "Allocating memory for images\n");
  char *imagemem = NULL;
  int imagememid;

  chk(is_AllocImageMem(
        camera->hCam, 
        camera->width,
        camera->height,
        3*8,      // bits per pixel
        &imagemem,
        &imagememid),camera);
  
  chk(is_SetImageMem(hCam, imagemem, imagememid),camera);

  camera->imagemem = imagemem;
  camera->imagememid = (unsigned int)imagememid;

  camera->status = CameraStatusConnected;
  return 0;
}

void cam_disconnect(Camera *camera) {
  if (camera->status == CameraStatusConnected) {
    chk(is_FreeImageMem(camera->hCam, 
                        camera->imagemem,
                        camera->imagememid),camera);

    chk(is_ExitCamera(camera->hCam),camera);
    fprintf(stderr, "Disconnected from camera %d\n", camera->hCam);
  }

  camera->status = CameraStatusDisconnected;
}

wchar_t *cam_capture(Camera *camera, wchar_t *path) {
  fprintf(stderr, "Capturing image\n");
UINT pixelClock = 0; 
 chk(is_PixelClock(camera->hCam, IS_PIXELCLOCK_CMD_GET,&pixelClock,sizeof(pixelClock)),camera);
fprintf(stderr,"pixelClock is %d\n",pixelClock);

  chk(is_FreezeVideo(camera->hCam, IS_WAIT),camera);
fprintf(stderr,"FreezeVideoComplete");
  if (path == NULL) path = L"snapshot.jpg";

  fprintf(stderr, "Saving image to %ls\n", path);
  IMAGE_FILE_PARAMS fparams;
  fparams.pwchFileName = path;
  fparams.nFileType = IS_IMG_JPG;
  fparams.ppcImageMem = &camera->imagemem;
  fparams.pnImageID = &camera->imagememid;

  chk(is_ImageFile(
        camera->hCam,
        IS_IMAGE_FILE_CMD_SAVE,
        &fparams,
        sizeof(fparams)),camera);
  return path;
}
