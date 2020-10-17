#ifndef ABERRATION_FIT_PROGRAM_H
#define ABERRATION_FIT_PROGRAM_H

#include "refinement.h"
#include <src/jaz/optics/aberrations_cache.h>
#include <src/jaz/math/tensor2x2.h>

// @TODO: move to jaz/optics/

class AberrationFitProgram : public RefinementProgram
{
	public:
		
		enum Granularity 
		{
			PerTomogram, Global
		};
		
		class EvenData
		{
			public:
				
				double Axx, Axy, Ayy, bx, by;
				
				EvenData& operator+=(const EvenData& d);
		};
		
		class OddData
		{
			public:
				
				double a;
				dComplex b;
				
				OddData& operator+=(const OddData& d);
		};

		class EvenSolution
		{
			public:

				BufferedImage<dComplex> optimum;
				BufferedImage<double> phaseShift;
				BufferedImage<Tensor2x2<double>> weight;
		};

		class OddSolution
		{
			public:

				BufferedImage<dComplex> optimum;
				BufferedImage<double> phaseShift;
				BufferedImage<double> weight;
		};
			 
		
		AberrationFitProgram(int argc, char *argv[]);
		

			int n_even, n_odd;
			bool do_even, do_odd;
		
		void readParams(IOParser& parser);
		void run();
				
		
		static void considerParticle(
				int part_id, 
				const Tomogram& tomogram, 
				const TomoReferenceMap& referenceMap, 
				const ParticleSet& dataSet,
				const AberrationsCache& aberrationsCache,
				bool flip_value,
				const BufferedImage<float>& frqWeight,
				int f0, int f1,
				BufferedImage<EvenData>& even_out, 
				BufferedImage<OddData>& odd_out);


		static EvenSolution solveEven(
				const BufferedImage<EvenData>& data);

		static std::vector<double> fitEven(
				const EvenSolution& solution,
				int n_bands,
				const std::vector<double>& initialCoeffs,
				double pixelSize,
				const std::string& prefix,
				bool writeImages);

		static gravis::d3Vector findAstigmatism(
				const EvenSolution& solution,
				const CTF& referenceCtf,
				double initialDeltaZ,
				double pixelSize,
				double initialStep);

		static BufferedImage<double> plotAstigmatism(
				const EvenSolution& solution,
				const CTF& referenceCtf,
				double initialDeltaZ,
				double range,
				double pixelSize,
				int size);

		static std::vector<double> solveAndFitEven(
				const BufferedImage<EvenData>& data,
				int n_bands,
				const std::vector<double>& initialCoeffs,
				double pixelSize,
				const std::string& prefix,
				bool writeImages);

		static double findDefocus(
				const BufferedImage<EvenData>& evenData,
				double pixelSize,
				const CTF& ctf0,
				double minDefocus,
				double maxDefocus,
				int steps);
		

		static OddSolution solveOdd(
				const BufferedImage<OddData>& data);

		static std::vector<double> fitOdd(
				const OddSolution& solution,
				int n_bands,
				const std::vector<double>& initialCoeffs,
				double pixelSize,
				const std::string& prefix,
				bool writeImages);

		static std::vector<double> solveAndFitOdd(
				const BufferedImage<OddData>& data,
				int n_bands,
				const std::vector<double>& initialCoeffs,
				double pixelSize,
				const std::string& prefix,
				bool writeImages);
};

#endif
