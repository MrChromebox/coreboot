#!/usr/bin/env bash
#
# SPDX-License-Identifier: GPL-2.0-only

set -e

# Default values
BASEBOARD=""
PLATFORM=""

# Parse command line arguments
while [[ $# -gt 0 ]]; do
	case $1 in
		-p|--platform)
			PLATFORM="$2"
			shift 2
			;;
		-h|--help)
			echo "Usage: $0 [OPTIONS] <baseboard>"
			echo ""
			echo "Extract blobs from all models in a baseboard recovery directory"
			echo ""
			echo "OPTIONS:"
			echo "  -p, --platform PLAT  Platform for ifdtool (REQUIRED)"
			echo "  -h, --help          Show this help message"
			echo ""
			echo "AVAILABLE PLATFORMS:"
			echo "  adl     - Alder Lake"
			echo "  aplk    - Apollo Lake"
			echo "  cnl     - Cannon Lake"
			echo "  lbg     - Lewisburg PCH"
			echo "  dnv     - Denverton"
			echo "  ehl     - Elkhart Lake"
			echo "  glk     - Gemini Lake"
			echo "  icl     - Ice Lake"
			echo "  ifd2    - IFDv2 Platform"
			echo "  jsl     - Jasper Lake"
			echo "  mtl     - Meteor Lake"
			echo "  sklkbl  - Sky Lake/Kaby Lake"
			echo "  tgl     - Tiger Lake"
			echo "  wbg     - Wellsburg"
			echo ""
			echo "EXAMPLES:"
			echo "  $0 -p mtl rex_recovery"
			echo "  $0 -p tgl nissa"
			echo "  $0 --platform adl rex_recovery"
			exit 0
			;;
		-*)
			echo "Error: Unknown option $1"
			echo "Use -h or --help for usage information"
			exit 1
			;;
		*)
			if [ -z "$BASEBOARD" ]; then
				BASEBOARD="$1"
			else
				echo "Error: Multiple baseboards specified"
				exit 1
			fi
			shift
			;;
	esac
done

# Check if baseboard parameter is provided
if [ -z "$BASEBOARD" ]; then
    echo "Error: You must provide a baseboard"
    echo "Use -h or --help for usage information"
    exit 1
fi

# Check if platform parameter is provided
if [ -z "$PLATFORM" ]; then
    echo "Error: You must provide a platform using -p or --platform"
    echo "Use -h or --help for usage information"
    exit 1
fi

# Check if baseboard directory exists
if [ ! -d "./$BASEBOARD" ]; then
    echo "Error: Baseboard directory './$BASEBOARD' does not exist"
    exit 1
fi

# Check if manifest.json exists
if [ ! -f "./$BASEBOARD/manifest.json" ]; then
    echo "Error: manifest.json not found in ./$BASEBOARD/"
    exit 1
fi

# Check if jq is available for JSON parsing
if ! command -v jq &> /dev/null; then
    echo "Warning: jq not found. Falling back to grep-based parsing."
    USE_JQ=false
else
    USE_JQ=true
fi

for MODEL in $(cat ./$BASEBOARD/VERSION | grep 'Model' | tr -s ' '| cut -d' ' -f2) ; do
        echo -e "Finding model: $MODEL"

        # Use jq for proper JSON parsing if available
        if [ "$USE_JQ" = true ]; then
            IMAGE=$(jq -r ".${MODEL,,}.host.image // empty" "./$BASEBOARD/manifest.json")
            if [ -z "$IMAGE" ]; then
                echo "Warning: No bios image found for model $MODEL in manifest.json"
                continue
            fi
        else
            # Fallback to grep-based parsing (original method)
            IMAGE="$(cat ./$BASEBOARD/manifest.json | grep -m1 -a9 ${MODEL,,} | grep 'images/bios' | cut -d '"' -f4)"
            if [ -z "$IMAGE" ]; then
                echo "Warning: No bios image found for model $MODEL"
                continue
            fi
        fi

        echo -e "Extracting image: $IMAGE"

        # Check if the image file exists
        if [ ! -f "./$BASEBOARD/$IMAGE" ]; then
            echo "Warning: Image file ./$BASEBOARD/$IMAGE does not exist, skipping"
            continue
        fi

	mkdir -p ./blobs-$BASEBOARD/${MODEL,,}
	./extract_blobs.sh -o ./blobs-$BASEBOARD/${MODEL,,} -p "$PLATFORM" ./$BASEBOARD/"$IMAGE"
done
echo ""
echo "All done"
