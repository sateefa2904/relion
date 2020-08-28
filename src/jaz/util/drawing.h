#ifndef DRAWING_TOOLS_H
#define DRAWING_TOOLS_H

#include <vector>
#include <string>
#include <src/jaz/image/buffered_image.h>

class Drawing
{
	public:

		template <typename T>
		static void drawCross(
				gravis::i2Vector location,
				T value,
				int size,
				RawImage<T>& target);

		template <typename PixelType, typename CoordType>
		static void drawCrosses(
				std::vector<gravis::t2Vector<CoordType>> locations,
				PixelType value,
				int size,
				RawImage<PixelType>& target);

		template <typename T>
		static void drawPoint(
				gravis::i2Vector location,
				T value,
				RawImage<T>& target);
};


template<typename T>
void Drawing::drawCross(
		gravis::i2Vector location, T value, int size, RawImage<T>& target)
{
	drawPoint(location, value, target);

	for (int i = 1; i < size-1; i++)
	{
		drawPoint(location + gravis::i2Vector(i,0),  value, target);
		drawPoint(location + gravis::i2Vector(0,i),  value, target);
		drawPoint(location + gravis::i2Vector(-i,0), value, target);
		drawPoint(location + gravis::i2Vector(0,-i), value, target);
	}
}

template<typename PixelType, typename CoordType>
void Drawing::drawCrosses(
		std::vector<gravis::t2Vector<CoordType> > locations,
		PixelType value,
		int size,
		RawImage<PixelType> &target)
{
	for (int i = 0; i < locations.size(); i++)
	{
		gravis::t2Vector<CoordType> pos0 = locations[i];

		gravis::i2Vector pos(
					(int)(std::round(pos0.x)),
					(int)(std::round(pos0.y)));

		drawCross(pos, value, size, target);
	}
}

template<typename T>
void Drawing::drawPoint(gravis::i2Vector location, T value, RawImage<T>& target)
{
	if (location.x >= 0 && location.x < target.xdim &&
		location.y >= 0 && location.y < target.ydim )
	{
		target(location.x, location.y) = value;
	}
}

#endif
