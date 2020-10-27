#include "TOP_CPlusPlusBase.h"
#include "cinder/app/AppNative.h"
#include "cinder/gl/Texture.h"
#include "cinder/Capture.h"
#include "cinder/params/Params.h"
#include "CinderOpenCV.h"
#include "opencv2/opencv.hpp"
#include "opencv2/gpu/gpu.hpp"
#include "opencv2/core/opengl_interop.hpp"

#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <gl/gl.h>

using namespace ci;
using namespace ci::app;
using namespace std;
using namespace gl;

class optFlowTOP : public TOP_CPlusPlusBase
{
public:
	optFlowTOP(const TOP_NodeInfo *info);
	virtual ~optFlowTOP();

	virtual void		getGeneralInfo(TOP_GeneralInfo *);
	virtual bool		getOutputFormat(TOP_OutputFormat*);


	virtual void		execute(const TOP_OutputFormatSpecs*,
								const TOP_InputArrays*,
								void* reserved);


	virtual int			getNumInfoCHOPChans();
	virtual void		getInfoCHOPChan(int index,
										TOP_InfoCHOPChan *chan);

	virtual bool		getInfoDATSize(TOP_InfoDATSize *infoSize);
	virtual void		getInfoDATEntries(int index,
											int nEntries,
											TOP_InfoDATEntries *entries);

	void				trackFeatures(cv::Mat currentFrame);
	void				chooseFeatures(cv::Mat mFrame);
	void				handleResize(unsigned int _inputWidth, unsigned int _inputHeight);

	int					myExecuteCount, pollInterval;
	unsigned int		myFBOIndex, myTextureIndex;
	unsigned int		inputWidth, inputHeight;

	gl::Texture			mTexture;
	cv::Mat				mPrevFrame;
	vector<cv::Point2f>	mPrevFeatures, mFeatures, mReturnFeatures, mDeltaFeatures;
	vector<uint8_t>		mFeatureStatuses;

	cv::gpu::GpuMat		mGpuCurFrame, mGpuPrevFrame;
	

	bool				mDrawPoints, mDrawVectors, drawInput;
	bool				autoFind, useGridPoints, useChopPoints;
	bool				useGPU;

	float				returnForce;


private:

	// We don't need to store this pointer, but we do for the example.
	// The TOP_NodeInfo class store information about the node that's using
	// this instance of the class (like its name).
	const TOP_NodeInfo		*myNodeInfo;

	static const int MAX_FEATURES = 1024;
	static const int SEARCH_WINDOW = 16;
	static const int MAX_VECTOR_COMPONENT = 48;

	float circleSize;

	unsigned int	pointsX, pointsY, pointsCount;
	unsigned int	autoRefreshInterval, maxAutoPoints, autoRefreshCount, inputChannelCount;

	void			resetGridPoints();
	void			applyReturnForce();
	cv::Size2i		searchWinSize;
	cv::TermCriteria termcrit;

};
