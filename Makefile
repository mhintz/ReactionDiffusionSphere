.PHONY: build run

all: build run

build:
	xcodebuild -configuration Debug -project xcode/ReactionDiffusionCubeMap.xcodeproj/

run:
	./xcode/build/Debug/ReactionDiffusionCubeMap.app/Contents/MacOS/ReactionDiffusionCubeMap
