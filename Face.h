//
//  Face.h
//  SDM
//
//  Created by Oriol Martinez Pujol on 26/4/18.
//  Copyright 2016 Universitat Pompeu Fabra, CMTech research group. All rights reserved.
//

#ifndef face_hpp
#define face_hpp

#include <opencv2/opencv.hpp>
#include <opencv2/objdetect.hpp>

#include <iostream>
#include <fstream>
#include <time.h>

using namespace std;
using namespace cv;

class Face {

public:

	/*************/
	/* VARIABLES */

	vector<cv::Mat> muS, eigS, muP, omegaD, omegaT, omegaF, omega3D;
	vector<cv::Point2f> meanPts;
	Rect pBox, fBox;
	int width, height;
	std::vector<int> scale = { 8 ,8 ,6 ,6 };
	vector<int> idxPts;

	/***********/
	/* METHODS */

	// Constructor
	Face(int patchsz, int descsz);
	// Patch Update
	void ipatch2tmplt(cv::Mat frame, cv::Mat& patch, cv::Mat& tform, cv::Mat& itform, cv::Rect origin);
	void ipatch2tmpltPro(cv::Mat frame, cv::Mat& patch, cv::Mat& tform, cv::Mat& itform, vector<cv::Point2f> wPts);
	// Haar face detection
	void detectFaces(cv::Mat grayframe, vector<Rect>& faces);
	// SDM regressors (landmarks)
	float estimateLandmarks(cv::Mat patch, std::vector<cv::Point2f>& esPts, vector<cv::Mat> omega);
	// Get bounding box from 2D landmarks
	CvRect landmarks2rect(std::vector<cv::Point2f> esPts);
	Mat procrustes(std::vector<cv::Point2f> esPts);

	// Destructor
	~Face();


private:

	/*************/
	/* VARIABLES */

	// Structure variables
	CascadeClassifier fd;

	// Mapping variables
	vector<Point2f> srcTri;
	vector<Point2f> dstTri;

	// HOG 
	// HOGDescriptor *hog;
	std::vector<cv::HOGDescriptor> hogs;
	std::vector<cv::Point> selPts;
	Mat phi, incPts, valPts;

	/***********/
	/* METHODS */

	// Read from binary files
	bool bin2mat(std::ifstream& ifs, std::vector<cv::Mat>& in_mat);

};


#endif /* face_hpp */