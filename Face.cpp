//
//  Face.c
//  SDM
//
//  Created by Oriol Martinez Pujol on 26/4/18.
//  Copyright 2016 Universitat Pompeu Fabra, CMTech research group. All rights reserved.
//

#include "Face.h"

/***************/
/* Constructor */

Face::Face(int patchsz, int descsz) {

	ifstream fmuS, feigS, fmuP, fomegaD, fomegaT, fomegaF, fomega3D, fmod, fang;

	
	// Determine the size of the template
	int pad = (int)floor(descsz / 2);

	// Face Box
	pBox = Rect(0, 0, patchsz + descsz, patchsz + descsz);

	// Face Box + padding
	fBox = Rect(pad, pad, patchsz, patchsz);

	// Set object width and height prop
	width = pBox.width;
	height = pBox.height;

	// Read binary models
	fmuP.open("models/muP.bin", ios::binary);
	fmuS.open("models/muS.bin", ios::binary);
	feigS.open("models/eigS.bin", ios::binary);
	fomegaD.open("models/omegaD.bin", ios::binary);
	fomegaT.open("models/omegaT.bin", ios::binary);
	fomegaF.open("models/omegaF.bin", ios::binary);
	fomega3D.open("models/omega3D.bin", ios::binary);


	// Map binary models to Mat or vector<Mat>
	bin2mat(fmuS, muS);
	bin2mat(feigS, eigS);
	bin2mat(fmuP, muP);
	bin2mat(fomegaD, omegaD);
	bin2mat(fomegaT, omegaT);
	bin2mat(fomegaF, omegaF);
	bin2mat(fomega3D, omega3D);

	
	// Inititalize HOG
	for (int i = 0; i < omegaT.size();i++) {
		int Csz = scale[i];
		int Bn = 4;
		int Bsz = Bn*Csz;
		int Bst = Csz;
		int nbins = 8;
		int Wsz = Bn*Csz;
		int Ws = Wsz;
		hogs.push_back(HOGDescriptor(cvSize(Wsz, Wsz), cvSize(Bsz, Bsz), cvSize(Bst, Bst), cvSize(Csz, Csz), nbins, 1, Ws, 0, 0.2, true, 64, true));
	}

	
	// Initialize and define affine mapping for template
	srcTri.push_back(Point2f(0, 0));
	srcTri.push_back(Point2f(1, 1));
	srcTri.push_back(Point2f(0, 1));

	dstTri.push_back(fBox.tl());
	dstTri.push_back(fBox.br());
	dstTri.push_back(Point2f((float)fBox.tl().x, (float)fBox.br().y));

	for (int x = 0; x < 68; x++) {
		meanPts.push_back(cv::Point2d(muP[0].at<float>(x) - 1, muP[0].at<float>(68 + x) - 1));
	}

	// Load face cascade xml
	fd.load("models/haarcascade_frontalface_default.xml");

	// Initialize descriptors and incPts
	idxPts = { 18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64,65,66,67,68 };
	incPts = Mat::zeros(1, 136, CV_32FC1);
	
	// Close file streams (not needed)
	fmuS.close();
	feigS.close();
	fmuP.close();
	fomegaD.close();
	fomegaT.close();
	fomegaF.close();
	
}

void Face::ipatch2tmplt(cv::Mat frame, cv::Mat& patch, cv::Mat& tform, cv::Mat& itform, cv::Rect origin) {

	// Set  3 points form both src (the image) to dst (tmplt domain)
	// origin points
	srcTri[0] = origin.tl();
	srcTri[1] = origin.br();
	srcTri[2] = Point2f((float)origin.tl().x, (float)origin.br().y);
	// Dst points
	dstTri[0] = fBox.tl();
	dstTri[1] = fBox.br();
	dstTri[2] = Point2f((float)fBox.tl().x, (float)fBox.br().y);

	// Estimate and apply the affine transformation
	tform = getAffineTransform(srcTri, dstTri);
	warpAffine(frame, patch, tform, patch.size(), CV_INTER_LINEAR + CV_WARP_FILL_OUTLIERS);

	// Compute the inverse for plotting the point to the frame
	invertAffineTransform(tform, itform);

}

void Face::ipatch2tmpltPro(cv::Mat frame, cv::Mat& patch, cv::Mat& tform, cv::Mat& itform, vector<cv::Point2f> wPts) {

	// Estimate and apply the affine transformation
	tform = procrustes(wPts);
	warpAffine(frame, patch, tform, patch.size(), CV_INTER_LINEAR + CV_WARP_FILL_OUTLIERS);

	// Compute the inverse for plotting the point to the frame
	invertAffineTransform(tform, itform);

}

void Face::detectFaces(cv::Mat grayframe, vector<Rect>& faces) {

	// Detect the face using Viola & Jones
	fd.detectMultiScale(grayframe, faces, 1.2, 2, 0, cvSize(100, 100));

}

/**************************/
/* Read from binary files */

bool Face::bin2mat(std::ifstream& ifs, std::vector<cv::Mat>& in_mat) {

	if (!ifs.is_open()) {
		return false;
	}

	int ncascade, rows, cols;
	ifs.read((char*)(&rows), sizeof(int));
	ifs.read((char*)(&cols), sizeof(int));
	ifs.read((char*)(&ncascade), sizeof(int));

	Mat tmp;

	for (int i = 0; i<ncascade; i++) {
		tmp.release();
		tmp.create(rows, cols, CV_32FC1);
		ifs.read((char*)(tmp.data), tmp.elemSize() * tmp.total());
		in_mat.push_back(tmp);
	}

	return true;
}

CvRect Face::landmarks2rect(std::vector<cv::Point2f> esPts) {

	CvRect rect = boundingRect(esPts);

	int pad;

	if (rect.width < rect.height) {
		pad = rect.height - rect.width;
		rect.x = rect.x - cvFloor(pad / 2);
		rect.width = rect.width + pad;
	}
	else {
		pad = rect.width - rect.height;
		rect.y = rect.y - cvFloor(pad / 2);
		rect.height = rect.height + pad;
	}

	return rect;

}

Mat Face::procrustes(std::vector<cv::Point2f> esPts) {


	std::vector<cv::Point2f> muPts = this->meanPts;

	Mat muPtsV = Mat::ones(idxPts.size(), 2, CV_32FC1);
	Mat esPtsV = Mat::ones(idxPts.size(), 2, CV_32FC1);

	for (int i = 0; i < idxPts.size(); ++i) {
		int idx = idxPts[i] - 1;
		muPtsV.at<float>(i, 0) = muPts[idx].x;
		muPtsV.at<float>(i, 1) = muPts[idx].y;
		esPtsV.at<float>(i, 0) = esPts[idx].x;
		esPtsV.at<float>(i, 1) = esPts[idx].y;
	}

	// 1st) Center pts
	cv::Mat dmean, omean;
	cv::reduce(muPtsV, dmean, 0, CV_REDUCE_AVG);
	subtract(muPtsV, cv::repeat(dmean, (int)muPtsV.rows, 1), muPtsV);
	cv::reduce(esPtsV, omean, 0, CV_REDUCE_AVG);
	subtract(esPtsV, cv::repeat(omean, (int)esPtsV.rows, 1), esPtsV);

	// 2nd) Normalize using Frobernius norm
	CvScalar dnorm = trace(muPtsV*muPtsV.t());
	CvScalar onorm = trace(esPtsV*esPtsV.t());

	muPtsV = muPtsV / (float)sqrt(dnorm.val[0]);
	esPtsV = esPtsV / (float)sqrt(onorm.val[0]);

	// 3rd) Find the rotation
	Mat  w, U, V;
	SVD::compute(muPtsV.t()*esPtsV, w, U, V);
	Mat rot = U * V;

	// 4th) Scaling
	reduce(w, w, 0, CV_REDUCE_SUM);
	float scale = w.at<float>(0) * ((float)sqrt(dnorm.val[0]) / (float)sqrt(onorm.val[0]));
	Mat c = dmean - scale*omean*rot.t();

	//Build the transformation matrix
	Mat tform = Mat(2, 3, CV_32FC1);
	tform.at<float>(0, 0) = scale * rot.at<float>(0, 0);
	tform.at<float>(0, 1) = scale * rot.at<float>(0, 1);
	tform.at<float>(1, 0) = scale * rot.at<float>(1, 0);
	tform.at<float>(1, 1) = scale * rot.at<float>(1, 1);
	tform.at<float>(0, 2) = c.at<float>(0);
	tform.at<float>(1, 2) = c.at<float>(1);

	return tform;

}

/************************/
/* Get Descriptors */

float Face::estimateLandmarks(cv::Mat patch, std::vector<cv::Point2f>& esPts, vector<cv::Mat> omega) {

	std::vector<cv::Point> selPts(idxPts.size());
	vector< float> descriptors;

	// for each cascade of the regressor
	for (int i = 0; i < omega.size(); i++) {

		for (int j = 0; j < idxPts.size(); j++) {
			selPts[j] = esPts[idxPts[j] - 1];
		}

		//OpenCV HOG
		hogs[i].compute(patch, descriptors, cv::Size(), cv::Size(), selPts);
		cv::vconcat(Mat::ones(1, 1, CV_32FC1), Mat(descriptors), phi);
		this->incPts = omega[i] * phi;

		float *pI = (float*)(this->incPts).data;

		for (int j = 0; j<meanPts.size(); j++) {

			esPts[j].x = (esPts[j].x - (*pI++));
			esPts[j].y = (esPts[j].y - (*pI++));

		}

		pI = NULL;

	}

	valPts = omegaF[0] * phi;
	float prob = 1 / (1 + exp(-valPts.at<float>(0)));

	return prob;

}

/**************/
/* Destructor */

Face::~Face() {

	// Clear vectors
	muS.clear();
	eigS.clear();
	muP.clear();
	omegaD.clear();
	omegaT.clear();
	omegaF.clear();

	meanPts.clear();
	srcTri.clear();
	dstTri.clear();

	// Clear mats
	incPts.release();
	valPts.release();

}
