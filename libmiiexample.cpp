#include <stdio.h>
#include <stdlib.h>
#include <wiiuse/wpad.h>
#include <ogc/video.h>
#include <mii.h>

static void *xfb = NULL;
static GXRModeObj *rmode = NULL;

int run = 1;

void init(){
	VIDEO_Init();
	WPAD_Init();
	rmode = VIDEO_GetPreferredMode(NULL);
	xfb = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));
	console_init(xfb,20,20,rmode->fbWidth,rmode->xfbHeight,rmode->fbWidth*VI_DISPLAY_PIX_SZ);
	VIDEO_Configure(rmode);
	VIDEO_SetNextFramebuffer(xfb);
	VIDEO_SetBlack(FALSE);
	VIDEO_Flush();
	VIDEO_WaitVSync();
	if(rmode->viTVMode&VI_NON_INTERLACE) VIDEO_WaitVSync();
	printf("\x1b[2;0H");
}


void showMii(Mii mii){
	printf("Press Left and Right on the Wii mote to navigate through your Miis\n\n");

	printf("Name: %s\n", mii.name);
	printf("By: %s\n", mii.creator);

	if (mii.female) printf("Gender: female\n");
	else printf("Gender: male\n");

	printf("Weight: %i\n", mii.weight);
	printf("Height: %i\n", mii.height);

	if (mii.favorite) printf("Mii is a Favorite\n");

	if (mii.month>0 && mii.day>0)
		printf("Birthday: %i/%i\n", mii.month, mii.day);

	if (mii.downloaded) printf("Downloaded\n");

	printf("Favorite Color: %i\n", mii.favColor);
	
	printf("Face is shape %i, color %i, with feature %i.\n", mii.faceShape, mii.skinColor, mii.facialFeature);
	
	if (mii.hairPart) printf("Hair is type %i, color %i, with a reversed part.\n", mii.hairType, mii.hairColor);
	else printf("Hair is type %i, color %i, with a normal part.\n", mii.hairType, mii.hairColor);
	
	printf("Eyebrows are type %i, color %i, rotated %i, size %i, %i high, with %i spacing.\n", mii.eyebrowType, mii.eyebrowColor, mii.eyebrowRotation, mii.eyebrowSize, mii.eyebrowVertPos, mii.eyebrowHorizSpacing);
	
	printf("Eyes are type %i, color %i, rotated %i, size %i, %i high, with %i spacing.\n", mii.eyeType, mii.eyeColor, mii.eyeRotation, mii.eyeSize, mii.eyeVertPos, mii.eyeHorizSpacing);
	
	printf("Nose is type %i, size %i, and %i high.\n", mii.noseType, mii.noseSize, mii.noseVertPos);
	
	printf("Lips are type %i, color %i, size %i, and %i high.\n", mii.lipType, mii.lipColor, mii.lipSize, mii.lipVertPos);
	
	if(mii.glassesType>0) printf("Mii has color %i glasses of %i type and %i size and they are %i high.\n", mii.glassesType, mii.glassesColor, mii.glassesSize, mii.glassesVertPos);
	
	if(mii.mustacheType>0) printf("Mii has a type %i mustache that is size %i and is %i high. It is of color %i.\n", mii.mustacheType, mii.mustacheSize, mii.mustacheVertPos, mii.facialHairColor);
	if(mii.beardType>0) printf("Mii has a type %i beard of %i color.\n", mii.beardType, mii.facialHairColor);

	if (mii.mole) printf("Has mole in position %i, %i and it is of size %i.\n", mii.moleHorizPos, mii.moleVertPos, mii.moleSize);

	printf("\n\nAll the above values are raw data that can be used for simple data display, data comparison, or to build a graphical representation of the mii using sprites of all the body parts!\n");
}

void clearScreen() {
	printf("\033[2J");printf("\x1b[2;0H");
}

int main() {
	init();

	Mii * miis;

	miis = loadMiis_Wii();

	int n = 0;

	showMii(miis[n]);

	while(run) {
		WPAD_ScanPads();
		u32 pressed = WPAD_ButtonsDown(0);
		if ((pressed & WPAD_BUTTON_RIGHT || pressed & WPAD_BUTTON_2 || pressed & WPAD_BUTTON_PLUS || pressed & WPAD_BUTTON_DOWN || pressed & WPAD_BUTTON_A) && n+1<NoOfMiis){
			clearScreen();
			n+=1;
			showMii(miis[n]);
		} else if ((pressed & WPAD_BUTTON_LEFT || pressed & WPAD_BUTTON_1 || pressed & WPAD_BUTTON_MINUS || pressed & WPAD_BUTTON_UP  || pressed & WPAD_BUTTON_B) && n>0) {
			clearScreen();
			n-=1;
			showMii(miis[n]);
		} else if (pressed & WPAD_BUTTON_HOME){
			clearScreen();
			printf("Goodbye!\n");
			run = 0;
		}
		VIDEO_WaitVSync();
	}
	return 0;
}
