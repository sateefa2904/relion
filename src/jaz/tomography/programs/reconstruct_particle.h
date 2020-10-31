#ifndef BACKPROJECT_PROGRAM_H
#define BACKPROJECT_PROGRAM_H

#include <string>
#include <src/jaz/image/buffered_image.h>
#include <src/jaz/tomography/optimisation_set.h>

class TomogramSet;
class ParticleSet;
class AberrationsCache;
class ParticleIndex;


class ReconstructParticleProgram
{
	public:
		
		ReconstructParticleProgram(){}
		

			OptimisationSet optimisationSet;
			std::string outDir, symmName;
			
			bool do_whiten, diag, no_subpix_off, explicit_gridding, no_reconstruction;
			int boxSize, cropSize, num_threads, outer_threads, inner_threads, max_mem_GB;
			double SNR, taper, binning;

		void readBasicParameters(int argc, char *argv[]);
		virtual void readParameters(int argc, char *argv[]);
		virtual void run();


	protected:

		void processTomograms(
				int first_t,
				int last_t,
				const TomogramSet& tomoSet,
				const ParticleSet& dataSet,
				const std::vector<std::vector<ParticleIndex>>& particles,
				const AberrationsCache& aberrationsCache,
				std::vector<BufferedImage<dComplex>>& dataImgFS,
				std::vector<BufferedImage<double>>& ctfImgFS,
				std::vector<BufferedImage<double>>& psfImgFS,
				const double binnedOutPixelSize,
				int s02D,
				bool do_ctf,
				bool flip_value,
				int verbosity);

		void finalise(
				std::vector<BufferedImage<dComplex>>& dataImgFS,
				std::vector<BufferedImage<double>>& ctfImgFS,
				std::vector<BufferedImage<double>>& psfImgFS,
				const double binnedOutPixelSize);

		void reconstruct(
				BufferedImage<double>& dataImgRS,
				BufferedImage<double>& dataImgDivRS,
				BufferedImage<double>& ctfImgFS,
				BufferedImage<double>* psfImgFS,
				BufferedImage<dComplex>& dataImgFS);

		void writeOutput(
				const BufferedImage<double>& corrected,
				const BufferedImage<double>& data,
				const BufferedImage<double>& weight,
				const std::string& tag,
				double pixelSize);
};

#endif
