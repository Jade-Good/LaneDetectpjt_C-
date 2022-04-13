#include <iostream>
#include "opencv2/opencv.hpp"

using namespace std;
using namespace cv;

Vec4f n_window_sliding(int left_start, int right_start, Mat roi, Mat v_thres, int w = 320, int h = 240, int nwindows = 8, int window_width = 40, int window_height = 30, int margin = 15) {
	vector<int> lx, ly, rx, ry;
	vector<Point> lpoints(nwindows), rpoints(nwindows);

	// window search
	for (int window = 0; window < nwindows; window++) {
		// window y up value / low value
		int win_y_high = h - (window + 1) * window_height;
		int win_y_low = h - window * window_height;


		int win_x_leftb_right = left_start + margin;
		int win_x_leftb_left = left_start - margin;

		int win_x_rightb_right = right_start + margin;
		int win_x_rightb_left = right_start - margin;

		// draw window at v_thres
		rectangle(roi, Rect(win_x_leftb_left, win_y_high, window_width, window_height), (255, 255, 255), 2);
		rectangle(roi, Rect(win_x_rightb_left, win_y_high, window_width, window_height), (255, 255, 255), 2);

		// box mid point
		int high = win_y_high + 4;

		int pixel_thres = window_width * 0.2;

		// window�� ��ġ�� ����ؼ� ���Ϳ� ��������� ���ʿ��� �κ��� ������ �� �ִ�. ������ 0�� ������ ���ϱ� ���� �����̹Ƿ� 0���� window_width ������ŭ ����
		int li = 0; int ll = 0, lr = 0;
		vector<int> lhigh_vector(window_width);
		for (auto x = win_x_leftb_left; x < win_x_leftb_right; x++) {
			li++;
			lhigh_vector[li] = v_thres.at<uchar>(high, x);

			// ������ �߾��� ����ϱ� ���� 255 �������� 255 ������ ���
			if (v_thres.at<uchar>(high, x) == 255 && ll == 0) {
				ll = x;
				lr = x;
			}
			if (v_thres.at<uchar>(high, x) == 255 && lr != 0) {
				lr = x;
			}
		}

		// window�ȿ��� 0�� �ƴ� �ȼ��� ������ ����
		int lnonzero = countNonZero(lhigh_vector);

		// 255�� �ȼ��� ������ threshold�� ������, ��� ���ߴ� 255 �ȼ� ���� ������ �� ������ �߾� ���� ���� window�� �߾����� ��´�.
		if (lnonzero > pixel_thres) {
			left_start = (ll + lr) / 2;
		}

		int ri = 0; int rl = 0, rr = 0;
		vector<int> rhigh_vector(window_width);
		for (auto x = win_x_rightb_left; x < win_x_rightb_right; x++) {
			ri++;
			rhigh_vector[ri] = v_thres.at<uchar>(high, x);
			if (v_thres.at<uchar>(high, x) == 255 && rl == 0) {
				rl = x;
				rr = x;
			}
			if (v_thres.at<uchar>(high, x) == 255 && lr != 0) {
				rr = x;
			}
		}

		int rnonzero = countNonZero(rhigh_vector);


		if (rnonzero > pixel_thres) {
			right_start = (rl + rr) / 2;
		}

		// ������ �������� ���ϱ� ���� vector, ����� Ư�� offset�� ���� ���� �ν��� ������ ���̹Ƿ� ��� x

		lpoints[window] = Point(left_start, (int)((win_y_high + win_y_low) / 2));
		rpoints[window] = Point(right_start, (int)((win_y_high + win_y_low) / 2));
	}

	Vec4f left_line, right_line;
	fitLine(lpoints, left_line, DIST_L2, 0, 0.01, 0.01); // ����� 0,1 ��° ���ڴ� ��������, 3,4��° ���ڴ� �� ���� �� ��
	fitLine(rpoints, right_line, DIST_L2, 0, 0.01, 0.01);


	int lx0 = left_line[2], ly0 = left_line[3]; // �� ���� �� ��
	int lx1 = lx0 - 300 * left_line[0], ly1 = ly0 - 300 * left_line[1]; // ���� ���� -> �׸����� �ϴ� ���̸� ���ְų� ������

	int rx0 = right_line[2], ry0 = right_line[3];
	int rx1 = rx0 - 300 * right_line[0], ry1 = ry0 - 300 * right_line[1];

	line(roi, Point(lx0, ly0), Point(lx1, ly1), Scalar(255, 0, 0), 2);
	line(roi, Point(rx0, ry0), Point(rx1, ry1), Scalar(255, 0, 0), 2);

	return left_line, right_line;
}

void draw_line(Mat frame, Mat roi, Vec4f left_line, Vec4f right_line, Mat per_mat_tosrc, int width = 640, int height = 480) {
	Mat newframe;
	warpPerspective(roi, newframe, per_mat_tosrc, Size(width, height), INTER_LINEAR);

	imshow("newframe", newframe);
}

int main()
{
	VideoCapture cap("../data/subProject.avi");

	if (!cap.isOpened()) {
		cerr << "Camera open failed" << endl;
		return -1;
	}

	// src image size
	int width = cvRound(cap.get(CAP_PROP_FRAME_WIDTH));
	int height = cvRound(cap.get(CAP_PROP_FRAME_HEIGHT));

	// warped image size
	int w = (int)width / 2, h = (int)height / 2;

	// point about warp transform
	vector<Point2f> src_pts(4);
	vector<Point2f> dst_pts(4);

	// �Ķ��� �� ���̰� �ϴ� roi
//	src_pts[0] = Point2f(0, 420); src_pts[1] = Point2f(213, 280); src_pts[2] = Point2f(395, 280); src_pts[3] = Point2f(595, 420);

	// �Ķ��� �� ���� roi
	src_pts[0] = Point2f(10, 395); src_pts[1] = Point2f(250, 250); src_pts[2] = Point2f(360, 250); src_pts[3] = Point2f(570, 395);
	dst_pts[0] = Point2f(0, h - 1); dst_pts[1] = Point2f(0, 0); dst_pts[2] = Point2f(w - 1, 0); dst_pts[3] = Point2f(w - 1, h - 1);

	// point about polylines
	vector<Point> pts(4);
	pts[0] = Point(10, 395); pts[1] = Point(250, 250); pts[2] = Point(360, 250); pts[3] = Point(570, 395);

	//	pts[0] = Point(0, 420); pts[1] = Point(213, 280); pts[2] = Point(395, 280); pts[3] = Point(595, 420);



	Mat per_mat_todst = getPerspectiveTransform(src_pts, dst_pts);
	Mat per_mat_tosrc = getPerspectiveTransform(dst_pts, src_pts);

	Mat frame, roi;
	while (true) {
		cap >> frame;

		if (frame.empty()) break;

		// perspective transform
		Mat roi;
		warpPerspective(frame, roi, per_mat_todst, Size(w, h), INTER_LINEAR);

		// roi box indicate
		polylines(frame, pts, true, Scalar(255, 255, 0), 2);


		// binary processing
#if 0	// full shot binary
		Mat hsv, v_thres;
		int lane_binary_thres = 140;
		cvtColor(frame, hsv, COLOR_BGR2HSV);
		vector<Mat> hsv_planes;
		split(frame, hsv_planes);
		Mat v_plane = hsv_planes[2];
		v_plane = 255 - v_plane;
		int means = mean(v_plane)[0];
		v_plane = v_plane + (128 - means);
		GaussianBlur(v_plane, v_plane, Size(), 1.0);
		inRange(v_plane, lane_binary_thres, 255, v_thres);
		imshow("v_thres", v_thres);

#elif 0	// 1-1 grayscale -> gaissian -> canny
		Mat roi_gray, roi_edge;
		cvtColor(roi, roi_gray, COLOR_BGR2GRAY);
		GaussianBlur(roi_gray, roi_gray, Size(), 1.0);

		threshold(roi_gray, roi_gray, 30, 75, THRESH_BINARY);

		Canny(roi_gray, roi_edge, 50, 150);
		imshow("roi", roi_gray);
		imshow("roi", roi_edge);


#elif 1	// 2-1 hsv -> gaussian -> inRange -> canny
		Mat hsv, v_thres;
		int lane_binary_thres = 155; // contrast : 185
		cvtColor(roi, hsv, COLOR_BGR2HSV);

		// split H/S/V
		vector<Mat> hsv_planes;
		split(roi, hsv_planes);
		Mat v_plane = hsv_planes[2];

		// inverse
		v_plane = 255 - v_plane;

		// brightness control
		int means = mean(v_plane)[0];
		// v_plane.convertTo(v_plane, -1, 1.5, 128 - means); // roi = roi + (128 - m);
		v_plane = v_plane + (128 - means);

		GaussianBlur(v_plane, v_plane, Size(), 1.0);

		inRange(v_plane, lane_binary_thres, 255, v_thres);


		//		imshow("hsv", hsv);
		//		imshow("v_plane", v_plane);
		imshow("v_thres", v_thres);


#elif 0	// 3-1 lab -> gaussian -> inrange -> canny
		Mat lab, lab_thres, roi_edge;
		int lane_binary_thres = 130;
		cvtColor(roi, lab, COLOR_BGR2Lab);

		GaussianBlur(roi, roi, Size(), 1.0);

		inRange(lab, Scalar(0, 0, lane_binary_thres), Scalar(255, 255, 255), lab_thres); //00130
		Canny(lab_thres, lab_edge, 50, 150);

		imshow("lab", lab);
		imshow("lab_thres", lab_thres);
		imshow("lab_edge", roi_edge);


#else	// grayscale -> sobel -> threshold
		Mat roi_gray;
		cvtColor(roi, roi_gray, COLOR_BGR2GRAY);

		Mat dx, dy;
		Sobel(roi, dx, CV_32FC1, 1, 0);
		Sobel(roi, dy, CV_32FC1, 0, 1);

		Mat mag;
		magnitude(dx, dy, mag);
		mag.convertTo(mag, CV_8UC1);

		Mat roi_edge = mag > 65;

		imshow("roi", roi);
		imshow("mag", mag);
		imshow("roi_edge", roi_edge);


#endif	

		// define constant for sliding window
		int nwindows = 8;
		int window_width = (int)(w / nwindows);
		int window_height = (int)(h / nwindows);
		int margin = window_width / 2;

		// define offset and draw line
		int offset = 180; // 228
		line(v_thres, Point(0, offset), Point(w, offset), (255, 0, 255), 1);

		// histogram -> ������ ���ؼ� ���� ���� ���� ã�� ���������� ��´�.
		vector<int> hist(w);
		for (int x = 0; x < w; x++) {
			for (int y = 0; y < h; y++) {
				hist[x] += v_thres.at<uchar>(y, x);
			}
		}

		int left_start = max_element(hist.begin(), hist.begin() + w / 2) - hist.begin();
		int right_start = max_element(hist.begin() + w / 2, hist.end()) - hist.begin();

		Vec4f left_line, right_line;
		left_line, right_line = n_window_sliding(left_start, right_start, roi, v_thres);

		draw_line(frame, roi, left_line, right_line, per_mat_tosrc);

		imshow("src", frame);
		imshow("roi", roi);

		if (waitKey(10) == 27) break;

	}
	cap.release();
	destroyAllWindows();
}
