#include <iostream>
#include <string>
#include <opencv2\opencv.hpp>
#include <opencv2\core.hpp>
#include <opencv2\highgui.hpp>
#include <opencv2\videoio.hpp>

extern "C" {
#include "vc.h"
}


int main(void) {
	char videofile[20] = "video.avi";

	cv::VideoCapture capture;
	
	struct
	{
		int width, height;
		int ntotalframes;
		int fps;
		int nframe;
	} video;

	std::string str;
	int key = 0;
	OVC* blob = nullptr;
	int nblob = 0;

	int raio = 0;

	int x_width = 0;
	int y_height = 0;
	int width_res = 0;
	int height_res = 60;

	int nlaranjas = 0;
	int xmin = 0;
	int xmax = 0;

	int calibre = 0;

	/* Leitura de vídeo de um ficheiro */
	capture.open(videofile);

	/* Verifica se foi poss�vel abrir o ficheiro de v�deo */
	if (!capture.isOpened())
	{
		std::cerr << "Erro ao abrir o ficheiro de v�deo!\n";
		return 1;
	}

	/* Número total de frames no vídeo */
	video.ntotalframes = (int)capture.get(cv::CAP_PROP_FRAME_COUNT);
	/* Frame rate do vídeo */
	video.fps = (int)capture.get(cv::CAP_PROP_FPS);
	/* Resolução do vídeo */
	video.width = (int)capture.get(cv::CAP_PROP_FRAME_WIDTH);
	video.height = (int)capture.get(cv::CAP_PROP_FRAME_HEIGHT);

	/* Cria uma janela para exibir o vídeo */
	cv::namedWindow("VC - VIDEO", cv::WINDOW_AUTOSIZE);

	cv::Mat frame;
	while (key != 'q') {
		/* Leitura de uma frame do vídeo */
		capture.read(frame);

		/* Verifica se conseguiu ler a frame */
		if (frame.empty()) break;

		/* Número da frame a processar */
		video.nframe = (int)capture.get(cv::CAP_PROP_POS_FRAMES);

		/* Exemplo de inserção texto na frame */
		str = std::string("RESOLUCAO: ").append(std::to_string(video.width)).append("x").append(std::to_string(video.height));
		cv::putText(frame, str, cv::Point(20, 25), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(0, 0, 0), 2);
		cv::putText(frame, str, cv::Point(20, 25), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(255, 255, 255), 1);
		str = std::string("TOTAL DE FRAMES: ").append(std::to_string(video.ntotalframes));
		cv::putText(frame, str, cv::Point(20, 50), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(0, 0, 0), 2);
		cv::putText(frame, str, cv::Point(20, 50), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(255, 255, 255), 1);
		str = std::string("FRAME RATE: ").append(std::to_string(video.fps));
		cv::putText(frame, str, cv::Point(20, 75), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(0, 0, 0), 2);
		cv::putText(frame, str, cv::Point(20, 75), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(255, 255, 255), 1);
		str = std::string("N. DA FRAME: ").append(std::to_string(video.nframe));
		cv::putText(frame, str, cv::Point(20, 100), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(0, 0, 0), 2);
		cv::putText(frame, str, cv::Point(20, 100), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(255, 255, 255), 1);
		
		/*Cria uma nova imagem IVC*/
		IVC* image = vc_image_new(video.width, video.height, 3, 255);
		IVC* image_2 = vc_image_new(video.width, video.height, 3, 255);
		IVC* image_3 = vc_image_new(video.width, video.height, 3, 255);
		IVC* image_4 = vc_image_new(video.width, video.height, 1, 255);
		IVC* image_5 = vc_image_new(video.width, video.height, 1, 255);

		/*Guarda a memória da imagem*/
		memcpy(image_2->data, frame.data, video.width* video.height * 3);

		/*Converte a codificação de cores BGR (video original) para RGB*/
		vc_convert_bgr_to_rgb(image_2,image_2);

		/*Converte a codificação de cores RGB para HSV*/
		vc_rgb_to_hsv(image_2, image_3);

		/*Faz a segmentação do frame atual que neste caso é HSV*/
		/*Estes valores de segmentação forma obtidos usando o GIMP num frame do vídeo*/
		/*Com a pipeta de cores recolheu-se a escala do laranja*/
		/*Isto vai permitir distinguir as laranjas das maçãs*/
		vc_hsv_segmentation(image_3, image_4, 10, 30, 75, 100, 30, 100);

		/*Faz a abertura binária da imagem*/
		vc_binary_open(image_4, image_5, 7, 7);

		/*Faz o fecho binário da imagem*/
		vc_binary_close(image_5, image_4, 3, 3);

		/*Prepara os bolbs para a sua etiquetagem*/
		blob = vc_binary_blob_labelling(image_4, image_5, &nblob);

		/*Faz a etiquetagem dos blobs*/
		vc_binary_blob_info(image_5, blob, nblob);

		y_height = (video.height / 2) - 30;
		width_res = video.width;

		/*Percorre o array de blobs*/
		for (int i = 0; i < nblob; i++)
		{
			int area_mm = (blob[i].area * 0.1964);
			int perimetro_mm = (blob[i].perimeter * 0.1964);
			int raio_mm = raio * 0.1964;
			int diametro_mm = raio_mm * 2;

			/*Verifica a área de um blob*/
			/*É uma verificação auxiliar face à segmentação, onde não é possive remover a maçã vermelha por completo*/
			if (blob[i].area > (video.width*video.height)*0.02) {
				int xr = (blob[i].xc - blob[i].x);
				int yr = (blob[i].yc - blob[i].y);

				/*Determina qual o maior dos raios*/
				if (xr < yr)
				{
					raio = xr;
				}
				else
				{
					raio = yr;
				}

				/*Produz uma linha no meio que vai ser capaz de contar o número de laranjas*/
				/*Utiliza o centro de massa de cada laranja como ponto de referência*/
				/*Para mais informações ver: https://tutorialedge.net/gamedev/aabb-collision-detection-tutorial/ */
				if (x_width < blob[i].xc &&
					x_width + width_res > blob[i].xc &&
					y_height < blob[i].yc &&
					y_height + height_res > blob[i].yc)
				{
					/*Dado que o centro de massa varia ligeiramente é necessário ciar uma escala*/
					/*Essa escala vai ter um determinado intervalo de vaores aonde o x e o y variam*/
					if (xmin == 0 && xmax == 0) {
						nlaranjas++;
						xmin = blob[i].xc - 15;
						xmax = blob[i].xc + 15;
					}

					if (xmin > blob[i].xc || xmax < blob[i].xc) {
						nlaranjas++;
						xmin = blob[i].xc - 15;
						xmax = blob[i].xc + 15;
					}
				}

				/*Determina o calibre de uma laranja usando o diametro*/
				/*Para cada diametro irá ter um calibre definido no regulamento*/
				if (diametro_mm >= 53 && diametro_mm < 60) {
					calibre = 13;
				}
				else if (diametro_mm >= 56 && diametro_mm < 63) {
					calibre = 12;
				}
				else if (diametro_mm >= 58 && diametro_mm < 66) {
					calibre = 11;
				}
				else if (diametro_mm >= 60 && diametro_mm < 68) {
					calibre = 10;
				}
				else if (diametro_mm >= 62 && diametro_mm < 70) {
					calibre = 9;
				}
				else if (diametro_mm >= 64 && diametro_mm < 73) {
					calibre = 8;
				}
				else if (diametro_mm >= 67 && diametro_mm < 76) {
					calibre = 7;
				}
				else if (diametro_mm >= 70 && diametro_mm < 80) {
					calibre = 6;
				}
				else if (diametro_mm >= 73 && diametro_mm < 84) {
					calibre = 5;
				}
				else if (diametro_mm >= 77 && diametro_mm < 88) {
					calibre = 4;
				}
				else if (diametro_mm >= 81 && diametro_mm < 92) {
					calibre = 3;
				}
				else if (diametro_mm >= 84 && diametro_mm < 96) {
					calibre = 2;
				}
				else if (diametro_mm >= 87 && diametro_mm < 100) {
					calibre = 1;
				}
				else if (diametro_mm >= 100) {
					calibre = 0;
				}
				

				/*Lista as informações pedidas sobre a laranja, e apresentas de forma ativa consoante os valores registados*/
				cv::circle(frame, cv::Point(blob[i].xc, blob[i].yc ), 6, cv::Scalar(255, 255, 255, 0), 3, 8, 0);
				cv::circle(frame, cv::Point(blob[i].xc, blob[i].yc), raio, cv::Scalar(255, 255, 255, 0), 3, 8, 0);
				str = std::string("Area: ").append(std::to_string(area_mm)).append("mm^2");
				cv::putText(frame, str, cv::Point(blob[i].xc - 90, blob[i].yc + 30), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(0, 0, 0), 2);
				str = std::string("Perimetro: ").append(std::to_string(perimetro_mm)).append("mm");
				cv::putText(frame, str, cv::Point(blob[i].xc - 90, blob[i].yc + 60), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(0, 0, 0), 2);
				str = std::string("Diametro: ").append(std::to_string(diametro_mm)).append("mm");
				cv::putText(frame, str, cv::Point(blob[i].xc - 90, blob[i].yc + 90), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(0, 0, 0), 2);
				str = std::string("Calibre: ").append(std::to_string(calibre));
				cv::putText(frame, str, cv::Point(blob[i].xc - 90, blob[i].yc + 120), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(0, 0, 0), 2);
				}

			/*Regista no canto inferior o número de laranjas*/
			str = std::string("NUM LARANJAS: ").append(std::to_string(nlaranjas));
			cv::putText(frame, str, cv::Point(20, 710), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(0, 0, 0), 2);
			cv::putText(frame, str, cv::Point(20, 710), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(255, 255, 255), 1);


		}
		
		/*Guarda um frame com imagem RGB para depois futuramente retirar informações usando o GIMP*/
		/*if (video.nframe == 233) {
			char nome[20] = "teste.ppm";
			vc_write_image(nome, image_2);
		}*/

		// Liberta a memória da imagem IVC que havia sido criada
		vc_image_free(image);
		
		// +++++++++++++++++++++++++

		/* Exibe a frame */
		cv::imshow("VC - VIDEO", frame);

		/* Sai da aplicação, se o utilizador premir a tecla 'q' */
		key = cv::waitKey(1);
	}

	/* Fecha a janela */
	cv::destroyWindow("VC - VIDEO");

	/* Fecha o ficheiro de vídeo */
	capture.release();

	return 0;
}