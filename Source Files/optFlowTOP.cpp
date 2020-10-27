#include "optFlowTOP.h"



// These functions are basic C function, which the DLL loader can find
// much easier than finding a C++ Class.
// The DLLEXPORT prefix is needed so the compile exports these functions from the .dll
// you are creating
extern "C"
{

	DLLEXPORT
		int
		GetTOPAPIVersion(void)
	{
		// Always return TOP_CPLUSPLUS_API_VERSION in this function.
		return TOP_CPLUSPLUS_API_VERSION;
	}

	DLLEXPORT
		TOP_CPlusPlusBase*
		CreateTOPInstance(const TOP_NodeInfo *info)
	{
		// Return a new instance of your class every time this is called.
		// It will be called once per TOP that is using the .dll
		return new optFlowTOP(info);

	}

	DLLEXPORT
		void
		DestroyTOPInstance(TOP_CPlusPlusBase *instance)
	{
		// Delete the instance here, this will be called when
		// Touch is shutting down, when the TOP using that instance is deleted, or
		// if the TOP loads a different DLL
		delete (optFlowTOP*)instance;
	}

};


optFlowTOP::optFlowTOP(const TOP_NodeInfo *info) : myNodeInfo(info)
{
	myExecuteCount = 0;
	myTextureIndex = 0;
	myFBOIndex = 0;
	inputWidth = 1280;
	inputHeight = 720;
	
	circleSize = 5.0;

	pointsX = 12;
	pointsY = 9;
	pointsCount = 0;

	returnForce = 0.05;
	autoRefreshInterval = 120;
	autoRefreshCount = 0;
	inputChannelCount = 0;

	mDrawPoints = true;
	mDrawVectors = true;
	autoFind = false;
	useGridPoints = true;
	useChopPoints = false;

	pollInterval = 30;

	searchWinSize = cv::Size2i(SEARCH_WINDOW, SEARCH_WINDOW);
	termcrit = cv::TermCriteria(CV_TERMCRIT_ITER | CV_TERMCRIT_EPS, 5, 0.1);

	resetGridPoints();
}

optFlowTOP::~optFlowTOP()
{
	
}

void
optFlowTOP::getGeneralInfo(TOP_GeneralInfo *ginfo)
{
	// Uncomment this line if you want the TOP to cook every frame even
	// if none of it's inputs/parameters are changing.
	ginfo->cookEveryFrame = false;
	ginfo->clearBuffers = false;
}

bool
optFlowTOP::getOutputFormat(TOP_OutputFormat *format)
{
	// In this function we could assign variable values to 'format' to specify
	// the pixel format/resolution etc that we want to output to.
	// If we did that, we'd want to return true to tell the TOP to use the settings we've
	// specified.
	// In this example we'll return false and use the TOP's settings
	return false;
}

void
optFlowTOP::execute(const TOP_OutputFormatSpecs* outputFormat, const TOP_InputArrays* arrays, void* reserved)
{
	unsigned int i;
	

	

	myTextureIndex = arrays->TOPInputs[0].textureIndex;
	myFBOIndex = outputFormat->FBOIndex;
	unsigned int _inputWidth = arrays->TOPInputs[0].width;
	unsigned int _inputHeight = arrays->TOPInputs[0].height;

	if ( (_inputWidth != inputWidth) || (_inputHeight != inputHeight) ) 
	{
		handleResize(_inputWidth, _inputHeight);
	}

	::glMatrixMode(GL_MODELVIEW);
	::glColor4f(1, 1, 1, 1);

	// Get the input texture and convert it for OpenCV
	mTexture = gl::Texture(GL_TEXTURE_2D, myTextureIndex, inputWidth, inputHeight, true);
	Surface mySurface(mTexture);
	cv::Mat currentFrame(toOcv(Channel(mySurface)));

	if (myExecuteCount % pollInterval == 0) {

		drawInput = arrays->floatInputs[0].values[0] > 0 ? true : false;
		mDrawPoints = arrays->floatInputs[0].values[1] > 0 ? true : false;
		mDrawVectors = arrays->floatInputs[0].values[2] > 0 ? true : false;

		returnForce = arrays->floatInputs[1].values[0];

		autoFind = arrays->floatInputs[2].values[0] > 0 ? true : false;
		maxAutoPoints = arrays->floatInputs[2].values[1];
		autoRefreshInterval = arrays->floatInputs[2].values[2];

		bool _useGridPoints = arrays->floatInputs[3].values[0] > 0 ? true : false;
		useChopPoints = arrays->floatInputs[4].values[0] > 0 ? true : false;

		if (useChopPoints) {
			// Use the CHOP's values for point positions
			useGridPoints = false;
			autoFind = false;
			unsigned int _inputChannelCount = arrays->CHOPInputs[0].numChannels;
			if (_inputChannelCount != inputChannelCount) {
				inputChannelCount = _inputChannelCount;
				mFeatures.resize(inputChannelCount / 2);
				mReturnFeatures.resize(inputChannelCount / 2);
				mDeltaFeatures.resize(inputChannelCount / 2);
			}
			for (i = 0; i < inputChannelCount / 2; i++) {
				float _x = arrays->CHOPInputs[0].channels[i * 2][0] * inputWidth;
				float _y = arrays->CHOPInputs[0].channels[i * 2 + 1][0] * inputHeight;

				if ((mReturnFeatures[i].x != _x) || (mReturnFeatures[i].y != _y)) {
					mReturnFeatures[i].x = _x;
					mReturnFeatures[i].y = _y;
					mFeatures[i].x = _x;
					mFeatures[i].y = _y;
					mPrevFeatures[i].x = _x;
					mPrevFeatures[i].y = _y;
					mDeltaFeatures[i].x = 0;
					mDeltaFeatures[i].y = 0;
				}
			}

		}
		else if (_useGridPoints) {

			float _pointsX = arrays->floatInputs[3].values[1];
			float _pointsY = arrays->floatInputs[3].values[2];
			if ((_pointsX != pointsX) || (_pointsY != pointsY)) {
				pointsX = _pointsX;
				pointsY = _pointsY;
				resetGridPoints();
			}
			else if (!useGridPoints) resetGridPoints();

			useGridPoints = true;
			autoFind = false;
		}
		//else if (autoFind) {
		//if (myExecuteCount % autoRefreshInterval == 0) {
		//if (myExecuteCount > 2) chooseFeatures(mPrevFrame);
		//}
		//}
	}
	

	// Put the OCVdata back into a Surface structure
	//Surface mySurface2(fromOcv(currentFrame));
	//gl::Texture mTexture2 = gl::Texture(mySurface2);
	//mTexture2.enableAndBind();

	// draw the incoming texure
	if (drawInput) {
		mTexture.enableAndBind();

		glBegin(GL_QUADS);
			glTexCoord2f(0, 1);
			glVertex2f(0, inputHeight);
			glTexCoord2f(1, 1);
			glVertex2f(inputWidth, inputHeight);
			glTexCoord2f(1, 0);
			glVertex2f(inputWidth, 0);
			glTexCoord2f(0, 0);
			glVertex2f(0, 0);
		glEnd();
	}

	myExecuteCount++;

	// Clear the output texture
	if (!drawInput && (mDrawPoints || mDrawVectors)) gl::clear();

	// Draw points
	if (mDrawPoints) {
		glDisable(GL_TEXTURE_2D);
		glEnable(GL_POINT_SMOOTH);
		glPointSize( circleSize );
		glColor4f(1, 0.2, 0.7, 0.33f);

		glBegin(GL_POINTS);
		for (vector<cv::Point2f>::const_iterator featureIt = mFeatures.begin(); featureIt != mFeatures.end(); ++featureIt) {
			cinder::Vec2f cen = fromOcv(*featureIt);
			glVertex2f(cen);
		}
		glEnd();
	}

	// Track the fetures at the points
	if (myExecuteCount > 2) {
		trackFeatures(currentFrame);
		applyReturnForce();
	}

	mPrevFrame = currentFrame;
	
}

int
optFlowTOP::getNumInfoCHOPChans()
{
	// We return the number of channel we want to output to any Info CHOP
	// connected to the TOP. In this example we are just going to send one channel.
	return 8;
}

void
optFlowTOP::getInfoCHOPChan(int index,
TOP_InfoCHOPChan *chan)
{
	// This function will be called once for each channel we said we'd want to return

	if (index == 0)
	{
		chan->name = "executeCount";
		chan->value = myExecuteCount;
	}
	else if (index == 1)
	{
		chan->name = "myTextureIndex";
		chan->value = myTextureIndex;
	}
	else if (index == 2)
	{
		chan->name = "myFBOIndex";
		chan->value = myFBOIndex;
	}
	else if (index == 3)
	{
		chan->name = "returnForce";
		chan->value = returnForce;
	}
	else if (index == 4)
	{
		chan->name = "autoRefreshCount";
		chan->value = autoRefreshCount;
	}
	else if (index == 5)
	{
		chan->name = "pointsCount";
		chan->value = pointsCount;
	}
	else if (index == 6)
	{
		chan->name = "inputChannelCount";
		chan->value = inputChannelCount;
	}
	else if (index == 7)
	{
		chan->name = "reserved2";
		chan->value = 0;
	}
}

bool		
optFlowTOP::getInfoDATSize(TOP_InfoDATSize *infoSize)
{
	infoSize->rows = 1 + pointsCount;
	infoSize->cols = 4;
	// Setting this to false means we'll be assigning values to the table
	// one row at a time. True means we'll do it one column at a time.
	infoSize->byColumn = false;
	return true;
}

void
optFlowTOP::getInfoDATEntries(int index,
										int nEntries,
										TOP_InfoDATEntries *entries)
{
	// It's safe to use static buffers here because Touch will make it's own
	// copies of the strings immediately after this call returns
	// (so the buffers can be reuse for each column/row)
	static char tempBuffer1[4096];
	static char tempBuffer2[4096];
	static char tempBuffer3[4096];
	static char tempBuffer4[4096];

	if (index == 0)
	{
		
		// Set the value for the first row
		strcpy(tempBuffer1, "x");
		entries->values[0] = tempBuffer1;

		strcpy(tempBuffer2, "y");
		entries->values[1] = tempBuffer2;

		strcpy(tempBuffer3, "dx");
		entries->values[2] = tempBuffer3;

		strcpy(tempBuffer4, "dy");
		entries->values[3] = tempBuffer4;
	}
	else if ((index > 0) && (myExecuteCount > 2)) {
		if (index <= mReturnFeatures.size()) {
			index -= 1;

			sprintf(tempBuffer1, "%f", mReturnFeatures[index].x);
			entries->values[0] = tempBuffer1;

			sprintf(tempBuffer2, "%f", mReturnFeatures[index].y);
			entries->values[1] = tempBuffer2;

			sprintf(tempBuffer3, "%f", mDeltaFeatures[index].x);
			entries->values[2] = tempBuffer3;

			sprintf(tempBuffer4, "%f", mDeltaFeatures[index].y);
			entries->values[3] = tempBuffer4;
		}
	}
}



void optFlowTOP::trackFeatures(cv::Mat currentFrame)
{
	vector<float> errors;
	//mPrevFeatures = mFeatures;
	if (!mFeatures.empty())
		cv::calcOpticalFlowPyrLK(mPrevFrame, currentFrame, mPrevFeatures, mFeatures, mFeatureStatuses, errors, searchWinSize, 0, termcrit);
		//cv::calcOpticalFlowPyrLK(mPrevFrame, currentFrame, mPrevFeatures, mFeatures, mFeatureStatuses, errors);
}

void optFlowTOP::chooseFeatures(cv::Mat mFrame) {
	autoRefreshCount++;
}

void optFlowTOP::handleResize(unsigned int _inputWidth, unsigned int _inputHeight)
{
	inputWidth = _inputWidth;
	inputHeight = _inputHeight;
	myExecuteCount = 0;

	if (useGridPoints) resetGridPoints();
}

void optFlowTOP::resetGridPoints() {
	unsigned int i, j;
	unsigned int xi, yj;

	mFeatures.resize(pointsX * pointsY);
	mPrevFeatures.resize(pointsX * pointsY);
	mReturnFeatures.resize(pointsX * pointsY);
	mDeltaFeatures.resize(pointsX * pointsY);

	for (i = 0; i<pointsX; i++) {
		for (j = 0; j<pointsY; j++) {
			xi = (i + 0.5) * inputWidth / pointsX;
			yj = (j + 0.5) * inputHeight / pointsY;

			mFeatures[j + i * pointsY] = cv::Point2f(xi, yj);
			mPrevFeatures[j + i * pointsY] = cv::Point2f(xi, yj);
			mReturnFeatures[j + i * pointsY] = cv::Point2f(xi, yj);
		}
	}

	pointsCount = pointsX * pointsY;
}

void optFlowTOP::applyReturnForce() {
	// Apply return force and possibly draw vectors
	glDisable(GL_TEXTURE_2D);
	glColor4f(0.1, 0.9, 0.2, 0.66f);

	float xx, yy;
	float xc, yc;
	float dx, dy;
	float ix, iy;
	int i;

	for (i = 0; i < mFeatures.size(); i++) {
		xc = mReturnFeatures[i].x;
		yc = mReturnFeatures[i].y;
		xx = mFeatures[i].x;
		yy = mFeatures[i].y;
		ix = mPrevFeatures[i].x;
		iy = mPrevFeatures[i].y;

		if ((xx < 1) || (yy < 1) || (xx > inputWidth) || (yy > inputHeight)) {
			mFeatures[i].x = ix;
			mFeatures[i].y = iy;
			xx = ix;
			yy = iy;
		}
		dx = xc - xx;
		dy = yc - yy;
		if ((std::abs(dx) > MAX_VECTOR_COMPONENT) || (std::abs(dy) > MAX_VECTOR_COMPONENT)) {
			mFeatures[i].x = ix;
			mFeatures[i].y = iy;
			xx = ix;
			yy = iy;
			dx = xc - xx;
			dy = yc - yy;
		}
		mDeltaFeatures[i] = cv::Point2f(dx * -1.0f, dy * -1.0f);

		if (mDrawVectors) {
			glBegin(GL_LINES);
			glVertex2f(xx, yy);
			glVertex2f(xc, yc);
			glEnd();
		}
		mPrevFeatures[i] = cv::Point2f(dx * returnForce + xx, dy * returnForce + yy);
	}
}

