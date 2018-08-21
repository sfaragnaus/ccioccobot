/* USAGE INFOS

 - Nella directory in cui � presente questa libreria dev'essere presente una directory 'haarcascades\'
    contenente i file xml con le informazioni necessarie ai vari classificatori

 - Per utilizzare i diversi classificatori � necessario attivarli, impostando a "true" le macro
    USING_ definite poco pi� in basso

 - Per visualizzare le informazioni di debug nel file di log bisogna impostare il valore della macro
	LOG_LEVEL a DEBUG

 - Per salvare le immagini relative alla porzione di immagine in cui viene effettuata la detezione da
	parte dei vari classificatori, � necessario impostare il valore della macro LOG_LEVEL a EVERYTHING_ENABLED

*/

/*
Codes for error generated by unpredictable situations that may occur during execution
This error codes will be written in the log file unless the program crashes

-- 1xx GASPARE ------------------------------

 -- detection -------------------------------
  101 ff_classifier try-catch undefined error
  102 pf_classifier try-catch undefined error
  103 fb_classifier try-catch undefined error
  104 ub_classifier try-catch undefined error
  105 lb_classifier try-catch undefined error

 -- identifying -----------------------------
*/

#include "recognition.h"
#include <iostream>
#include <opencv2/opencv.hpp>

// -------------------------------------------------------------------------------------------------
// Inclusion of some opencv libraries
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/objdetect/objdetect.hpp>
#include <opencv2/imgproc/imgproc.hpp>

// Some macros with names of classifiers used to detect the user
#define FRONTAL_FACE_MODEL "haarcascades\\haarcascade_frontalface_alt2.xml"
#define PROFILE_FACE_MODEL "haarcascades\\haarcascade_profileface.xml"
#define FULL_BODY_MODEL    "haarcascades\\haarcascade_fullbody.xml"
#define UPPER_BODY_MODEL   "haarcascades\\haarcascade_upperbody.xml"
#define LOWER_BODY_MODEL   "haarcascades\\haarcascade_lowerbody.xml"

#define USING_FF_CLASSIFIER true
#define USING_PF_CLASSIFIER true
#define USING_FB_CLASSIFIER false
#define USING_UB_CLASSIFIER true
#define USING_LB_CLASSIFIER true

// LOG FILE
#define LOG_FILE_NAME    "log_recognition_dll.txt"								// Name of the log file
#define DATE_TIME_FORMAT "%y-%m-%d %I:%M%p "									// Format to use to write date and time to the log file

#define LOG_DIV_LINE writeLog(string("-------------------------------------------------------------------------------------"), NO_LOG);

// Set of possible log levels
enum LOG_LINE_LEVEL { EVERYTHING_ENABLED	=   0 ,
					  DEBUG					=   5 ,
					  WARNING				=  10 ,
					  ERROR					=  15 ,
					  NO_LOG				= 100 };

// LOG_LEVEL indicates the minimum line level for which it will be written a line to the log file
const enum LOG_LINE_LEVEL LOG_LEVEL = WARNING;

// getCurrentDateTime() returns a string containing current date and time formatted like DATE_TIME_FORMAT
std::string getCurrentDateTime();

// writeLog() writes a line to the log file like: "<date and time> [llevel] - <text>"
void writeLog(std::string & text, enum LOG_LINE_LEVEL llevel = DEBUG);

// Prototype for the fuctions used to detect people
void detectPeople(cv::Mat);

void setupClassifier(cv::CascadeClassifier & classifier, std::string model, std::string cname_for_log);
std::vector<cv::Rect> detectWithClassifier(cv::Mat frame, cv::CascadeClassifier & cclassifier, std::string short_name_for_log, unsigned int * tot_det, int ecode);
// -------------------------------------------------------------------------------------------------

bool setup(void)
{
	using namespace std;

	if (LOG_LEVEL < NO_LOG)
	{
		ofstream log_file(LOG_FILE_NAME, ios_base::out);
		log_file << endl;
	}

	// 1 - setup the classifiers
	// 2 - complete initial setup

	return true;
}

void kinematicInfo(float *speed, float *acc, float *angular, float *angularAcc, float delta)
{
	*speed = 0;
	*acc = 0;
	*angular = 0;
	*angularAcc = 0;
}

void captureImage(unsigned char *pixelDataSx, unsigned char *pixelDataDx, int w, int h, int stride, float delta)
{
	//sx cam
	cv::Mat camSx(h, w, CV_8UC3, pixelDataSx);
	cv::Mat flippedSx;
	cv::flip(camSx, flippedSx, 0);
	cv::cvtColor(flippedSx, flippedSx, cv::COLOR_RGB2BGR);

	//dx cam
	cv::Mat camDx(h, w, CV_8UC3, pixelDataDx);
	cv::Mat flippedDx;
	cv::flip(camDx, flippedDx, 0);
	cv::cvtColor(flippedDx, flippedDx, cv::COLOR_RGB2BGR);

	//DO NOT ENABLE THESE UNLESS FOR TESTING PURPOSE!
	//cv::imwrite("leftcam.jpg", flippedSx);
	//cv::imwrite("rightcam.jpg", flippedDx);

	// -------------------------------------------------------------------------------------------------
	detectPeople(flippedSx);
	// -------------------------------------------------------------------------------------------------
}

// -------------------------------------------------------------------------------------------------
void detectPeople(cv::Mat frame_in)
{
	using namespace std;
	using namespace cv;

	// The actual classifiers
	static cv::CascadeClassifier ff_classifier; // frontal face classifier
	static cv::CascadeClassifier pf_classifier; // profile face classifier
	static cv::CascadeClassifier fb_classifier; // full body classifier
	static cv::CascadeClassifier ub_classifier; // upper body classifier
	static cv::CascadeClassifier lb_classifier; // lower body classifier

	// DEBUG: The output frame used to see detection results
	Mat frame_out;

	// Vectors of rects returned by the classifiers
	vector<Rect> ff_detected;
	vector<Rect> pf_detected;
	vector<Rect> fb_detected;
	vector<Rect> ub_detected;
	vector<Rect> lb_detected;

	static unsigned int tot_det_ff = 0;
	static unsigned int tot_det_pf = 0;
	static unsigned int tot_det_fb = 0;
	static unsigned int tot_det_ub = 0;
	static unsigned int tot_det_lb = 0;
	
	// this code section must stay here untill simulator doesn't call setup()
	{
		// Quit application unless loading classifier models can be completed
		if (USING_FF_CLASSIFIER && ff_classifier.empty()) setupClassifier(ff_classifier, FRONTAL_FACE_MODEL, "ff_model");
		if (USING_PF_CLASSIFIER && pf_classifier.empty()) setupClassifier(pf_classifier, PROFILE_FACE_MODEL, "pf_model");
		if (USING_FB_CLASSIFIER && fb_classifier.empty()) setupClassifier(fb_classifier, FULL_BODY_MODEL,    "fb_model");
		if (USING_UB_CLASSIFIER && ub_classifier.empty()) setupClassifier(ub_classifier, UPPER_BODY_MODEL,   "ub_model");
		if (USING_LB_CLASSIFIER && lb_classifier.empty()) setupClassifier(lb_classifier, LOWER_BODY_MODEL,   "lb_model");
	}
	
	if (frame_in.empty())
	{
		writeLog(string("Received an empty frame!!"), WARNING);
		return;
	}

	cvtColor(frame_in, frame_in, COLOR_BGR2GRAY);
	equalizeHist(frame_in, frame_in);
	writeLog(string("converted input image from BGR to grayscale"));

	if (USING_FF_CLASSIFIER) ff_detected = detectWithClassifier(frame_in, ff_classifier, "ff", &tot_det_ff, 101);
	if (USING_PF_CLASSIFIER) pf_detected = detectWithClassifier(frame_in, pf_classifier, "pf", &tot_det_pf, 102);
	if (USING_FB_CLASSIFIER) fb_detected = detectWithClassifier(frame_in, fb_classifier, "fb", &tot_det_fb, 103);
	if (USING_UB_CLASSIFIER) ub_detected = detectWithClassifier(frame_in, ub_classifier, "ub", &tot_det_ub, 104);
	if (USING_LB_CLASSIFIER) lb_detected = detectWithClassifier(frame_in, lb_classifier, "lb", &tot_det_lb, 105);

	//imwrite("output_image.jpg", frame_out);

	LOG_DIV_LINE
}

void writeLog(std::string & text, enum LOG_LINE_LEVEL llevel)
{
	using namespace std;
	
	// Do not write if LOG_LEVEL is greater than the line level
	if (llevel < LOG_LEVEL) return;

	// Prepare a string with an indication of the line level
	string log_level;
	switch (llevel)
	{
	case DEBUG:
		log_level = "[D]";
		break;
	case WARNING:
		log_level = "[W]";
		break;
	case ERROR:
		log_level = "[E]";
		break;
	case NO_LOG:
	default: // Shouldn't be triggered
		log_level = "[] ";
		break;
	}
	
	// Open a stream to the log file
	ofstream log_file(LOG_FILE_NAME, ios_base::out | ios_base::app);
	log_file << getCurrentDateTime();	// write the current date and time
	log_file << log_level;				// write the line level
	log_file << " - ";					// write a separator
	log_file << text;					// write the log text
	log_file << endl;					// flush
}

std::string getCurrentDateTime()
{
	const int buff_size = 80;
	time_t rawtime;
	struct tm * timeinfo;
	char st[buff_size];

	time(&rawtime);
	timeinfo = localtime(&rawtime);

	strftime(st, buff_size, DATE_TIME_FORMAT, timeinfo);

	return st;
}

void setupClassifier(cv::CascadeClassifier & cclassifier, std::string model, std::string cname_for_log)
{
	using namespace std;

	ostringstream oss;
	oss << "trying to load: ";
	oss << model;
	writeLog(oss.str());

	// reset ostringstream (oss)
	oss.str(""); oss.clear();

	if (!cclassifier.load(model))
	{
		oss << "Error loading";
		oss << cname_for_log;
		oss << "!";
		writeLog(oss.str(), ERROR);
		
		assert(false);
	}
	else
	{
		oss << cname_for_log;
		oss << " loaded correctly";
		writeLog(oss.str());
	}

	LOG_DIV_LINE
}

std::vector<cv::Rect> detectWithClassifier(cv::Mat frame, cv::CascadeClassifier & cclassifier, std::string short_name_for_log, unsigned int * tot_det, int ecode)
{
	using namespace std;
	using namespace cv;

	ostringstream oss;
	vector<Rect> detected;

	try
	{
		//lb_classifier.detectMultiScale(frame, lb_detected, 1.1, 2, 0 | CASCADE_SCALE_IMAGE, Size(30, 30));
		cclassifier.detectMultiScale(frame, detected, 1.1, 2);

		oss << "detected ";
		oss << detected.size();
		oss << " body parts using ";
		oss << short_name_for_log;
		oss << " classifier";
		writeLog(oss.str());
	}
	catch (cv::Exception e)
	{
		oss << short_name_for_log;
		oss << "_classifier.detectMultiScale thrown: ";
		oss << e.err;
		writeLog(oss.str(), ERROR);
	}
	catch (...)
	{
		oss << "Something wrong appened (error code: ";
		oss << ecode;
		oss << ")...";

		writeLog(oss.str(), ERROR);
	}

	for (size_t i = 0; i < detected.size(); ++i)
	{
		ostringstream file_name;
		file_name << "detection_";
		file_name << short_name_for_log;
		file_name << "_";
		file_name << (*tot_det < 100 ? "0" : "");
		file_name << (++(*tot_det) < 10 ? "0" : "");
		file_name << *tot_det;
		file_name << ".jpg";

		if (LOG_LEVEL == EVERYTHING_ENABLED)
		{
			imwrite(file_name.str(), frame(detected.at(i)));
			writeLog("written: " + file_name.str());
		}
	}

	return detected;
}
// -------------------------------------------------------------------------------------------------
