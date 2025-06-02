#include <stdint.h>

#define HOLOCAM_IMPLEMENTATION
#include "holo_cam.h"

#define WIDTH 1920
#define HEIGHT 1080

int
main(int argc, char** argv)
{
	Holo_Settings settings = {0};
	if (Holo_Init(settings))
	{
		Holo_Camera_Reader* webcam_reader = 0;
		unsigned int webcam_names_len = 0;
		Holo_Camera_Name* webcam_names = Holo_GetCameraNames(&webcam_names_len);

		if (webcam_names != 0)
		{
			webcam_reader = HoloCameraReader_Create(webcam_names[0].symbolic_name, WIDTH, HEIGHT, 30);
		}

		Holo_Cam* cam = HoloCam_Create(L"Holo Cam 0", 1920, 1080, 30, 3009);

		if (webcam_reader != 0 && cam != 0 && HoloCam_Start(cam))
		{

			while (true)
			{
				static uint32_t rgb_frame[WIDTH*HEIGHT];

				HoloCameraReader_ReadFrame(webcam_reader, rgb_frame);

				/*
				for (unsigned int j = 0; j < HEIGHT; ++j)
				{
					for (unsigned int i = 0; i < WIDTH; ++i)
					{
						uint32_t color = rgb_frame[j*WIDTH + i];

						float r = ((color >> 16) & 0xFF) / 255.0f;
						float g = ((color >>  8) & 0xFF) / 255.0f;
						float b = ((color >>  0) & 0xFF) / 255.0f;

						r = (int)(r*2)/2.0f;
						g = (int)(g*2)/2.0f;
						b = (int)(b*2)/2.0f;

						rgb_frame[j*WIDTH + i] = (uint32_t)(uint8_t)(r*255) << 16 |
							                       (uint32_t)(uint8_t)(g*255) <<  8 |
																		 (uint32_t)(uint8_t)(b*255);
					}
				}*/

				HoloCam_Present(cam, rgb_frame);
			}
		}

		HoloCam_Destroy(&cam);
		HoloCameraReader_Destroy(&webcam_reader);

		Holo_Cleanup();
	}

	return 0;
}
