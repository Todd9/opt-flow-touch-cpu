# opt-flow-touch-cpu
Optical flow CPlusPlus TOP. Uses OpenCV (2.4.9) and Cinder (0.8.6)

Originally compiled with Visual Studio 2013 to create an x64 DLL for use in TouchDesigner 64 bit, version 088. Still works in 088 but for some reason makes 099 crash on startup. Hopefully we'll get the crash issure resolved so this can be useful again.

Created in 2015 for VSquared Labs

How to use:
A CHOP sets the custom parameters (such as how many points to track), while an Info DAT receives the output (vectors that describe the optical flow). See the TOE example.

Demo video: https://vimeo.com/114542945
