#include "tomogram.h"
#include "projection_IO.h"
#include "tomo_ctf_helper.h"
#include <src/jaz/optics/damage.h>
#include <src/jaz/image/power_spectrum.h>
#include <src/jaz/tomography/particle_set.h>


using namespace gravis;


Tomogram::Tomogram()
{

}

d4Matrix Tomogram::getProjectionMatrix(int f) const
{

    std::cerr << "TODO: check correction of this code!!!!" << std::endl;
    if (optics.pixelSize < 0.001)
        REPORT_ERROR("BUG: Tomogram::getProjectionMatrix encountered pixel size of zero");

    d4Matrix s0, s1, s2, r0, r1, r2;

    // Get specimen center
    t3Vector<double> specimen_center((double)int(w0/2), (double)int(h0/2), (double)int(d0/2) );
    s0 = s0. translation(-specimen_center);

    // Get specimen shifts (in pixels)
    t3Vector<double> specimen_shifts(xshift_angst[f] / optics.pixelSize, yshift_angst[f] / optics.pixelSize, 0.);
    s1 = s1.translation(specimen_shifts);

    // Get tilt image center
    std::vector<long int> tilt_image_center_int = stack.getSizeVector();
    t3Vector<double> tilt_image_center((double)int(tilt_image_center_int[0]/2), (double)int(tilt_image_center_int[1]/2), 0.);
    s2 = s2.translation(tilt_image_center);

    // Get rotation matrices
    t3Vector<double> xaxis(1., 0., 0.), yaxis(0., 1., 0.), zaxis(0., 0., 1.);
    r0 = r0.rotation(xaxis, xtilt[f]);
    r1 = r1.rotation(yaxis, ytilt[f]);
    r2 = r2.rotation(zaxis, zrot[f]);

    return s2 * s1 * r2 * r1 * r0 * s0;

    /*
    Matrix2D< RFLOAT > T0, T, T1, Rx, Ry, Rz, All;
    translation3DMatrix(-w0/2, -h0/2, -d0/2, T0);
    translation3DMatrix(xshift, yshift, 0., T);
    translation3DMatrix(stack.xdim/2, stack.ydim/2, 0., T1);
    rotation3DMatrix(xtilt, 'X', Rx);
    rotation3DMatrix(ytilt, 'Y', Ry);
    rotation3DMatrix(zrot, 'Z', Rz);
    //All = T1 * T * Rz * Ry * Rx * T0;
    All = T * Rz * Ry * Rx;

    d4Matrix result;
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++)
            result(i,j) = All(i, j);

    return result;
     */

}
void Tomogram::setProjectionAnglesFromMatrix(int f)
{

    RFLOAT abs_sb, sign_sb;
    d4Matrix& A = projectionMatrices[f];

    // From https://www.geometrictools.com/Documentation/EulerAngles.pdf
    RFLOAT thetaX, thetaY, thetaZ;
    if (A(2,0) < 0.99999)
    {
        if (A(2,0) > -0.99999)
        {
            thetaX = atan2(A(2,1), A(2,2));
            thetaY = asin (-A(2,0));
            thetaZ = atan2(A(1,0), A(0,0));
        }
        else // A(2,0) = -1
        {
            thetaX = 0.;
            thetaY = PI/2.;
            thetaZ = -atan2(-A(1,2), A(1,1));
        }
    }
    else // A(2,0) = +1
    {
            thetaX = 0.;
            thetaY = -PI/2.;
            thetaZ = atan2(-A(1,2), A(1,1));
    }

    xtilt[f] = RAD2DEG(thetaX);
    ytilt[f] = RAD2DEG(thetaY);
    zrot[f] = RAD2DEG(thetaZ);

    // Also get centers


}

d2Vector Tomogram::projectPoint(const d3Vector& p, int frame) const
{
	const d2Vector pl = (projectionMatrices[frame] * d4Vector(p)).xy();
	
	if (hasDeformations)
	{
		return imageDeformations[frame]->apply(pl);
	}
	else
	{
		return pl;
	}
}

d2Vector Tomogram::projectPointDebug(const d3Vector &p, int frame) const
{
	const d2Vector pl = (projectionMatrices[frame] * d4Vector(p)).xy();

	std::cout << p << " -> " << pl << '\n';
	std::cout << projectionMatrices[frame] << '\n';

	if (hasDeformations)
	{
		d2Vector p1 = imageDeformations[frame]->apply(pl);
		std::cout << " -> " << p1 << "(" << (p1 - pl) << ")\n";

		return p1;
	}
	else
	{
		std::cout << "\n";

		return pl;
	}
}

bool Tomogram::isVisible(const d3Vector& p, int frame, double radius) const
{
	d2Vector q = projectPoint(p, frame);

    // SHWS 2feb24: now q is in centered coordinates, so add half the imageSize!
    //q.x += imageSize.x/2.;
    //q.y += imageSize.y/2.;

	return     q.x > radius && q.x < imageSize.x - radius
			&& q.y > radius && q.y < imageSize.y - radius;
}

bool Tomogram::isVisibleAtAll(const std::vector<d3Vector> &trajectory, double radius) const
{
	std::vector<bool> vis = determineVisiblity(trajectory, radius);

	bool all_outside = true;

	for (int i = 0; i < vis.size(); i++)
	{
		if (vis[i])
		{
			all_outside = false;
			break;
		}
	}

	return !all_outside;
}

bool Tomogram::getVisibilityMinFramesMaxDose(const std::vector<d3Vector> &trajectory, double radius, double max_dose, int min_frames, std::vector<bool> &isVisible) const
{

    if (min_frames < 0) REPORT_ERROR("ERROR: min_frames should be larger than zero!");

    isVisible = determineVisiblity(trajectory, radius);

    if (max_dose > 0.)
        for (int f = 0; f < frameCount; f++)
            if (getCumulativeDose(f) > max_dose) isVisible[f] = false;

    int sum_is_visible = 0;
    for (int f = 0; f < frameCount; f++)
        if (isVisible[f]) sum_is_visible += 1;

    return (sum_is_visible >= min_frames);

}

std::vector<bool> Tomogram::determineVisiblity(const std::vector<d3Vector>& trajectory, double radius) const
{
	const int fc = trajectory.size();
	
	std::vector<bool> out(fc);

	for (int f = 0; f < fc; f++)
	{
		out[f] = isVisible(trajectory[f], f, radius);
	}
	
	return out;
}

double Tomogram::getFrameDose() const
{
	return fractionalDose;
	//cumulativeDose[frameSequence[1]] - cumulativeDose[frameSequence[0]];
}
double Tomogram::getCumulativeDose(int frame) const
{
    return cumulativeDose[frame];
}
BufferedImage<float> Tomogram::computeDoseWeight(int boxSize, double binning) const
{
	// @TODO: add support for B/k factors
	
	return Damage::weightStack_GG(cumulativeDose, optics.pixelSize * binning, boxSize);
}

BufferedImage<float> Tomogram::computeNoiseWeight(int boxSize, double binning, double overlap) const
{
	const int s0 = (int)(binning * boxSize + 0.5);
	const int s = boxSize;
	const int sh = s/2 + 1;
	const int fc = stack.zdim;

	BufferedImage<float> out(sh, s, fc);

	for (int f = 0; f < fc; f++)
	{
        BufferedImage<double> powSpec = PowerSpectrum::periodogramAverage2D(
                stack, s0, s0, overlap, f, false);

        std::vector<double> powSpec1D = RadialAvg::fftwHalf_2D_lin(powSpec);

        std::vector<float> frqWghts1D(powSpec1D.size());

        for (int i = 0; i < powSpec1D.size(); i++)
        {
            if (powSpec1D[i] > 0.0)
            {
                frqWghts1D[i] = (float)(1.0 / sqrt(powSpec1D[i]));
            }
            else
            {
                frqWghts1D[i] = 0.f;
            }
        }

        RawImage<float> outSlice = out.getSliceRef(f);
        RadialAvg::toFftwHalf_2D_lin(frqWghts1D, sh, s, outSlice, binning);
    }

	return out;
}

double Tomogram::getDepthOffset(int frame, d3Vector position) const
{
	const d4Matrix& projFrame = projectionMatrices[frame];
	d4Vector pos2D = projFrame * d4Vector(position);
    // SHWS 2feb24: undo centering in relion5
    d4Vector cent2D = projFrame * d4Vector(centre);
	return pos2D.z - cent2D.z;

    //return pos2D.z;
}

CTF Tomogram::getCtf(int frame, d3Vector position) const
{
	double dz_pos = getDepthOffset(frame, position);
	double dz = handedness * optics.pixelSize * defocusSlope * dz_pos;

	CTF ctf = centralCTFs[frame];

	ctf.DeltafU += dz;
	ctf.DeltafV += dz;

	ctf.initialise();

	return ctf;
}

int Tomogram::getLeastDoseFrame() const
{
	return IndexSort<double>::sortIndices(cumulativeDose)[0];
}

d3Vector Tomogram::computeCentreOfMass(
		const ParticleSet& particleSet,
		const std::vector<ParticleIndex>& particle_indices) const
{
	const int pc = particle_indices.size();

	d3Vector centre_of_mass(0.0, 0.0, 0.0);

	for (int p = 0; p < pc; p++)
	{
		const ParticleIndex particle_id = particle_indices[p];
		const d3Vector pos = particleSet.getPosition(particle_id);
		centre_of_mass += pos;
	}

	centre_of_mass /= pc;

	return centre_of_mass;
}

Tomogram Tomogram::extractSubstack(d3Vector position, int width, int height) const
{
	Tomogram out = *this;

	out.stack.resize(width, height, frameCount);

	for (int f = 0; f < frameCount; f++)
	{
		const d4Vector pf = projectionMatrices[f] * d4Vector(position);

		const int x0 = (int)(pf.x - width/2  + 0.5);
		const int y0 = (int)(pf.y - height/2 + 0.5);

		for (int y = 0; y < height; y++)
		for (int x = 0; x < width;  x++)
		{
			out.stack(x,y,f) = stack(x0+x, y0+y, f);
		}

		out.projectionMatrices[f](0,3) -= x0;
		out.projectionMatrices[f](1,3) -= y0;
	}

	return out;
}

Tomogram Tomogram::FourierCrop(double factor, int num_threads, bool downsampleData) const
{
	Tomogram out = *this;

	if (downsampleData && hasImage)
	{
		out.stack = Resampling::FourierCrop_fullStack(stack, factor, num_threads, true);
	}
	else
	{
		out.stack.resize(0,0,0);
		out.hasImage = false;
	}

	for (int f = 0; f < frameCount; f++)
	{
		out.projectionMatrices[f] /= factor;
	}

	out.optics.pixelSize *= factor;

	return out;
}

bool Tomogram::hasFiducials() const
{
	return fiducialsFilename.length() > 0 && fiducialsFilename != "empty";
}

bool Tomogram::validateParticleOptics(
		const std::vector<ParticleIndex>& particleIds,
		const ParticleSet& particleSet)
{
	const int gc = particleSet.numberOfOpticsGroups();

	std::vector<bool> valid(gc, false);

	for (int p = 0; p < particleIds.size(); p++)
	{
		const int g = particleSet.getOpticsGroup(particleIds[p]);

		if (!valid[g])
		{
			const double eps = 1e-3;

			bool Cs_good = true;

			if (particleSet.optTable.labelExists(EMDL_CTF_CS))
			{
				const double Cs_particles = particleSet.optTable.getDouble(EMDL_CTF_CS, g);
				const double Cs_tomogram = optics.Cs;

				Cs_good = std::abs(Cs_tomogram - Cs_particles) < eps;
			}

            bool Q0_good = true;

            if (particleSet.optTable.labelExists(EMDL_CTF_Q0))
            {
                const double Q0_particles = particleSet.optTable.getDouble(EMDL_CTF_Q0, g);
                const double Q0_tomogram = optics.Q0;

                Q0_good = std::abs(Q0_tomogram - Q0_particles) < eps;
            }

			bool u_good = true;

			if (particleSet.optTable.labelExists(EMDL_CTF_VOLTAGE))
			{
				const double u_particles = particleSet.optTable.getDouble(EMDL_CTF_VOLTAGE, g);
				const double u_tomogram = optics.voltage;

				u_good = std::abs(u_particles - u_tomogram) < eps;
			}

			bool s_good = true;

			if (particleSet.optTable.labelExists(EMDL_TOMO_TILT_SERIES_PIXEL_SIZE))
			{
				const double s_particles = particleSet.optTable.getDouble(EMDL_TOMO_TILT_SERIES_PIXEL_SIZE, g);
				const double s_tomogram = optics.pixelSize;

				s_good = std::abs(s_particles - s_tomogram) < eps;
			}

			if (Cs_good && u_good && s_good && Q0_good)
			{
				valid[g] = true;
			}
			else
			{
				REPORT_ERROR_STR("Tomogram::validateParticleOptics: inconsistent values between optics and tomograms tables.");
			}
		}
	}

	return true;
}

BufferedImage<int> Tomogram::findDoseXRanges(const RawImage<float> &doseWeights, double cutoffFraction)
{
	const int sh = doseWeights.xdim;
	const int s = doseWeights.ydim;
	const int fc = doseWeights.zdim;

	BufferedImage<int> out(s,fc);

	for (int f = 0; f < fc; f++)
	{
		for (int y = 0; y < s; y++)
		{
			out(y,f) = 0;

			const double yy = y < s/2? y : y - s;
			const double xmax = sqrt(s*s/4 - yy*yy);

			for (int x = 0; x < sh && x <= xmax; x++)
			{
				const float dw = doseWeights(x,y,f);

				if (dw > cutoffFraction)
				{
					out(y,f) = x+1;
				}
			}
		}
	}

	return out;
}
