
/**
 * ExpansionMath.c
 *
 * DayZ Expansion Mod
 * www.dayzexpansion.com
 * © 2022 DayZ Expansion Mod Team
 *
 * This work is licensed under the Creative Commons Attribution-NonCommercial-NoDerivatives 4.0 International License.
 * To view a copy of this license, visit http://creativecommons.org/licenses/by-nc-nd/4.0/.
 *
*/

class ExpansionMath
{
	static const float OneOverLN2 = 1.4426950408889634;
	static const float Epsilon = 0.000000001;

	static float LinearConversion(float minFrom, float maxFrom, float value, float minTo = 0, float maxTo = 1, bool clamp = true)
	{
		float newValue = (((value - minFrom) * (maxTo - minTo)) / (maxFrom - minFrom)) + minTo;

		if (clamp)
			newValue = Clamp(newValue, minTo, maxTo);

		return newValue;
	}

	static float PowerConversion(float minFrom, float maxFrom, float value, float minTo = 0, float maxTo = 1, float power = 1.0)
	{
		value = Math.Clamp((value - minFrom) / (maxFrom - minFrom), 0, 1); //! value must be equal to or between 0 and 1
		return (1 - Math.Pow(1 - value, power)) * (maxTo - minTo) + minTo;
	}

	static float Clamp( float value, float edge0 = 0, float edge1 = 1 )
	{
		float min = Math.Min(edge0, edge1);
		float max = Math.Max(edge0, edge1);
		return Math.Clamp(value, min, max);
	}

	//! Returns binomial coefficient without explicit use of factorials,
	//! which can't be used with negative integers
	static float PascalTriangle(int a, int b)
	{
		float result = 1;
		for (int i = 0; i < b; ++i)
		{
			result *= (a - i) / (i + 1);
		}
		return result;
	}

	//! Generalized smoothstep https://en.wikipedia.org/wiki/Smoothstep#Generalization_to_higher-order_equations
	//! This function is well-behaved up to including N=5. Beyond that, errors accumulate, until the function eventually breaks.
	static float SmoothStep(float x, int N = 1, float edge0 = 0, float edge1 = 1)
	{
		x = Math.Clamp((x - edge0) / (edge1 - edge0), 0, 1); //! x must be equal to or between 0 and 1
		float result = 0;
		for (int n = 0; n <= N; ++n)
		{
			result += PascalTriangle(-N - 1, n) * PascalTriangle(2 * N + 1, N - n) * Math.Pow(x, N + n + 1);
		}
		return edge0 + result * (edge1 - edge0);
	}

	//! Alternative to smoothstep
	static float SCurve(float x, float edge0 = 0, float edge1 = 1)
	{
		x = Math.Clamp((x - edge0) / (edge1 - edge0), 0, 1); //! x must be equal to or between 0 and 1
		float result = 0;
		if (x < 0.5)
            result = 2 * Math.Pow(x, 2);
        else
            result = 1 - 2 * Math.Pow(1 - x, 2);
		return edge0 + result * (edge1 - edge0);
	}

	static vector InterpolateAngles(vector from, vector to, float time, float mult = 4.0, float pow = 2.0)
	{
		float diff;
		float f;

		vector result;

		for (int i = 0; i < 3; i++)
		{
			diff = Math.AbsFloat(AngleDiff2(from[i], to[i]));
			if (diff > 22.5)
				f = mult - PowerConversion(0.0, 1.0, 22.5 / diff, mult, 0.0, pow);
			else
				f = mult;
			result[i] = LinearConversion(0.0, 1.0, time * f, from[i], to[i]);
		}

		return result;
	}

	static vector ExRotateAroundPoint(vector point, vector pos, vector axis, float cosAngle, float sinAngle)
    {
        return vector.RotateAroundZero(pos - point, axis, cosAngle, sinAngle) + point;
    }

	//! Generate random point at angle in dregrees
	static vector GetRandomPointAtDegrees(vector center, float degrees, float minRadius, float maxRadius)
	{
		float theta = degrees * Math.DEG2RAD;

		return GetRandomPointAtRadians(center, theta, minRadius, maxRadius);
	}

	//! Generate random point at angle in radians
	static vector GetRandomPointAtRadians(vector center, float theta, float minRadius, float maxRadius)
	{
		float maxRadiusSq = maxRadius * maxRadius;
		float minRadiusSq = minRadius * minRadius;
		float r = Math.Sqrt(Math.RandomFloat01() * (maxRadiusSq - minRadiusSq) + minRadiusSq);

		return Vector(center[0] + r * Math.Cos(theta), center[1], center[2] + r * Math.Sin(theta));
	}

	//! Generate random point within ring (uniformly)
	static vector GetRandomPointInRing(vector center, float minRadius, float maxRadius)
	{
		float theta = Math.RandomFloat01() * 2 * Math.PI;

		return GetRandomPointAtRadians(center, theta, minRadius, maxRadius);
	}

	//! Generate random point within circle (uniformly)
	static vector GetRandomPointInCircle(vector center, float radius)
	{
		float r = radius * Math.Sqrt( Math.RandomFloat01() );
		float theta = Math.RandomFloat01() * 2 * Math.PI;

		return Vector(center[0] + r * Math.Cos(theta), center[1], center[2] + r * Math.Sin(theta));
	}

	//! Converts angle in interval [0,360] to interval [-180,180]
	static float RelAngle(float angle)
	{
		if (angle > 180)
			return angle - 360;
		return angle;
	}

	//! Converts angle in interval [-180,180] to interval [0,360]
	static float AbsAngle(float angle)
	{
		if (angle < 0)
			return angle + 360;
		return angle;
	}

	/**
	\brief Returns smallest possible angle difference
		\param a \p float Angle in interval [0,360]
		\param b \p float Target angle in interval [0,360]
		\return \p float - Angle difference in interval [-180,180]
		@code
			Print( ExpansionMath.AngleDiff2(270.0, 0.0) );
			Print( ExpansionMath.AngleDiff2(350.0, 1.0) );
			Print( ExpansionMath.AngleDiff2(60.0, 275.0) );

			>> 90.0
			>> 11.0
			>> -145.0
		@endcode
	*/
	static float AngleDiff2(float a, float b)
	{
		float d = b - a;
		if (d > 180)
			return d - 360;
		else if (d < -180)
			return d + 360;
		return d;
	}

	/**
	 * @brief Smooth values (moving average)
	 * 
	 * @param passses   Number of passes
	 * @param weights   Float array containing weighting factors. Its length
	 *                  determines the size of the window to use.
	 *                  Defaults to {1.0, 1.0, 1.0}
	 * @param protect   Int array containing indexes of values to protect
	 */
	static TFloatArray MovingAvg(TFloatArray values, int passes = 1, TFloatArray weights = null, TIntArray protect = null)
	{
		TFloatArray data();
		TFloatArray window();

		if (!weights || weights.Count() < 3 || weights.Count() % 2 != 1)
		{
			if (weights)
				CF_Log.Warn("Invalid weight count %1, has to be uneven and >= 3 - using default weights {1, 1, 1}", weights.Count().ToString());
			window.Insert(1.0);
			window.Insert(1.0);
			window.Insert(1.0);
		}
		else
		{
			window.Copy(weights);
		}

		data.Copy(values);

		for (int npass = 0; npass < passes; npass++)
		{
			_MovingAvg(data, window, protect);
		}

		return data;
	}

	private static void _MovingAvg(inout TFloatArray data, TFloatArray window, TIntArray protect)
	{
		for (int j = 0; j < data.Count(); j++)
		{
			float v = data[j];
			TFloatArray tmpwindow();
			tmpwindow.Copy(window);
			if (j > 0 && j < data.Count() - 1 && (!protect || protect.Find(j) == -1))
			{
				while (tmpwindow.Count() >= 3)
				{
					int tl = (tmpwindow.Count() - 1) / 2;
					// Print(j + " " + tl + " " + tmpwindow);
					if (tl > 0 && j - tl >= 0 && j + tl <= data.Count() - 1)
					{
						TFloatArray windowslice();
						for (int k = j - tl; k < j + tl + 1; k++)
							windowslice.Insert(data[k]);
						float windowsize = 0;
						float tmpwindowsum = 0;
						for (int l = 0; l < tmpwindow.Count(); l++)
						{
							windowsize += tmpwindow[l] * windowslice[l];
							tmpwindowsum += tmpwindow[l];
						}
						v = windowsize / tmpwindowsum;
						break;
					}
					else
					{
						tmpwindow.RemoveOrdered(0);
						tmpwindow.Remove(tmpwindow.Count() - 1);
					}
				}
			}
			data[j] = v;
		}
	}

	/**
	 * @brief Interpolate path
	 * 
	 * @param path        Array of vectors
	 * @param curveType   The type of curve that should be used for interpolation
	 * @param smooth      If path should be smoothed after applying curve interpolation (may help against overly sharp changes in angle)
	 */
	static TVectorArray PathInterpolated(TVectorArray path, ECurveType curveType = ECurveType.CatmullRom, bool smooth = true)
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.PATH_INTERPOLATION, ExpansionMath);
#endif

		if (path.Count() < 3)
			return path;

		float minDistance = 4.472136;

		TVectorArray points();
		TVectorArray interpolatedPath();

#ifdef DIAG_DEVELOPER
		EXTrace.Print(EXTrace.PATH_INTERPOLATION, null, "Original points");
#endif

		foreach (vector pathPoint: path)
		{
			points.Insert(Vector(pathPoint[0], pathPoint[2], 0));
			
#ifdef DIAG_DEVELOPER
			EXTrace.Print(EXTrace.PATH_INTERPOLATION, null, pathPoint[0].ToString() + " " + pathPoint[2].ToString());
#endif
		}

		//! 1st pass - insert linearly interpolated points roughly every 100 m where needed to make path more evenly spaced
		//! (helps against overly sharp turns in curve interpolation)
		TVectorArray intermediatePoints = {points[0]};
		
#ifdef DIAG_DEVELOPER
		EXTrace.Print(EXTrace.PATH_INTERPOLATION, null, "Intermediate points");
		EXTrace.Print(EXTrace.PATH_INTERPOLATION, null, points[0][0].ToString() + " " + points[0][1].ToString());
#endif

		for (int i = 1; i < points.Count(); i++)
		{
			float pointDistance = vector.Distance(points[i - 1], points[i]);
			int steps;

			if (pointDistance >= 200.0)
				steps = Math.Floor(pointDistance / 100.0);
			else
				steps = 1;

			for (int j = 1; j <= steps; j++)
			{
				vector intermediatePoint = vector.Lerp(points[i - 1], points[i], j / steps);
				intermediatePoints.Insert(intermediatePoint);

#ifdef DIAG_DEVELOPER
				EXTrace.Print(EXTrace.PATH_INTERPOLATION, null, intermediatePoint[0].ToString() + " " + intermediatePoint[1].ToString());
#endif
			}
		}

		//! Curve interpolation
#ifdef DIAG_DEVELOPER
		EXTrace.Print(EXTrace.PATH_INTERPOLATION, null, "Filtered interpolated points - " + typename.EnumToString(ECurveType, curveType));
#endif

		float t;
		float step = 0.01;
		float tEnd = points.Count() - 1;
		vector previousPoint3D;
		vector curveDir2D;
		float previousAngle;
		float angleDiff;
		float distance;

		while (t < tEnd)
		{
			vector curvePoint = Math3D.Curve(curveType, t / tEnd, intermediatePoints);

//#ifdef DIAG_DEVELOPER
			//EXTrace.Print(EXTrace.PATH_INTERPOLATION, null, curvePoint[0].ToString() + " " + curvePoint[1].ToString());
//#endif

			vector point3D = Vector(curvePoint[0], 0, curvePoint[1]);

			float tCur = t;
			t += step;

			if (tCur)
				curveDir2D = vector.Direction(previousPoint3D, point3D);
	
			previousPoint3D = point3D;

			if (tCur)
			{
				distance += curveDir2D.Length();

				if (/*tCur != Math.Round(tCur) &&*/ tCur < tEnd)
				{
					float angle = curveDir2D.VectorToAngles()[0];
					angleDiff += Math.AbsFloat(angle - previousAngle);
					previousAngle = angle;
					if (angleDiff < 5.0)
						continue;
				}
			}

			if (!tCur || /*tCur == Math.Round(tCur) ||*/ distance > minDistance || tCur == tEnd)
			{
				distance = 0;
				angleDiff = 0;

				interpolatedPath.Insert(point3D);
#ifdef DIAG_DEVELOPER
				EXTrace.Print(EXTrace.PATH_INTERPOLATION, null, point3D[0].ToString() + " " + point3D[2].ToString());
#endif
			}
		}

		if (smooth)
		{
			//! Smoothing pass - moving avg
#ifdef DIAG_DEVELOPER
			EXTrace.Print(EXTrace.PATH_INTERPOLATION, null, "Smoothing pass - moving avg");
#endif

			TFloatArray x();
			TFloatArray z();

			foreach (vector xyz: interpolatedPath)
			{
				x.Insert(xyz[0]);
				z.Insert(xyz[2]);
			}

			x = ExpansionMath.MovingAvg(x);
			z = ExpansionMath.MovingAvg(z);

			for (int k = 0; k < interpolatedPath.Count(); k++)
			{
				vector smoothedPoint = Vector(x[k], 0, z[k]);
				interpolatedPath[k] = smoothedPoint;

#ifdef DIAG_DEVELOPER
				EXTrace.Print(EXTrace.PATH_INTERPOLATION, null, x[k].ToString() + " " + z[k].ToString());
#endif
			}
		}

		//! Final pass - set Y to surface
		for (int l = 0; l < interpolatedPath.Count(); l++)
		{
			vector interpolatedPoint = interpolatedPath[l];
			interpolatedPoint[1] = GetGame().SurfaceY(interpolatedPath[l][0], interpolatedPath[l][2]);
			interpolatedPath[l] = interpolatedPoint;
#ifdef DIAG_DEVELOPER
			EXTrace.Print(EXTrace.PATH_INTERPOLATION, null, interpolatedPath[l].ToString(false));
#endif
		}

		return interpolatedPath;
	}

	//! @brief Compares two numbers
	//! Returns 1 if a > b, zero if a == b and -1 if a < b
	//! This function can be used to work-around broken integer comparison involving negative numbers in EnForce
	//! https://feedback.bistudio.com/T167065
	static int Cmp(int a, int b)
	{
		if (a == b)
		{
			return 0;
		}
		else if ((a >= 0 && b >= 0) || (a < 0 && b < 0))
		{
			if (a > b)
				return 1;
			else
				return -1;
		}
		else if (a < 0 && b >= 0)
		{
			return -1;
		}

		return 1;
	}

	/**
	 * @brief Proper RandomFloatInclusive that does include endpoint, other than the one in EnMath
	 * 
	 * @note uniform = true assumes Math.RandomInt is indeed uniform
	 * @note with uniform = true, the range of possible unique values is limited to 2^24
	 *       (otherwise 2^26 in best-case scenarios)
	 * @note if the range defined by min and max exceeds 2^24, distribution is no longer uniform
	 */
	static float RandomFloatInclusive(float min, float max, bool uniform = false)
	{
		int max_range = 16777216;  //! 2^24 (uniform, unique)
		float range = max - min;

		if (!uniform || Math.AbsFloat(range) > max_range)
			max_range = 1073741824;  //! 2^30 (non-uniform, 2^26 unique values)

		int random_int = Math.RandomIntInclusive(0, max_range);
		float rand_float = (float)random_int / (float)max_range;

		return min + (rand_float * range);
	}

	static float FMod(float x, float y)
	{
		float r = x / y;

		if (r >= 1.0)
		{
			//PrintFormat("FMOD %1 >= 1.0", r);
			//return x - Math.Floor(r) * y;  //! Math.Floor is broken, e.g. float.MAX gets turned into 1.70141e+38, half of what it should be
			return x + Math.Ceil(-r) * y;  //! Gives correct floor
		}
		else if (r <= -1.0)
		{
			//PrintFormat("FMOD %1 <= -1.0", r);
			return x - Math.Ceil(r) * y;
		}

		//PrintFormat("FMOD -1.0 < %1 < 1.0", r);
		return x;
	}

	//! Proper Log2. Vanilla Math.Log2 signature gives the impression it works with floats. It doesn't, it's a purely integer implementation.
	//! Adapted from https://stackoverflow.com/a/3719696
	//! @note tolerance sets the accuracy of the result and should not be lower than 1e-15 (no accuracy improvement past that point)
	static float Log2(float x, float tolerance = 1e-13)
	{
		if (!x)
		{
			Error("Math domain error");
			return "nan".ToFloat();
		}

		//! Shortcut for x == 1.0
		if (x == 1.0)
			return 0.0;

		//! Shortcut for x == 0.5
		if (x == 0.5)
			return -1.0;

		float log2x = 0.0;

		//! Integer part
		while (x < 1.0)
		{
			log2x -= 1.0;
			x *= 2.0;
		}
		while (x >= 2.0)
		{
			log2x += 1.0;
			x /= 2.0;
		}

		//! Fractional part
		float frac = 1.0;
		while (frac >= tolerance)
		{
			frac /= 2.0;
			x *= x;
			if (x >= 2.0)
			{
				x /= 2.0;
				log2x += frac;
			}
		}

		return log2x;
	}

	static float Log10(float x, float tolerance = 1e-13)
	{
		static const float Log2_10 = Log2(10);
		return Log2(x) / Log2_10;
	}
	
	static float NewtonNext(float r, float x)
	{
		int ir = r;
		int div = 1 << ir;
		return r - OneOverLN2 * (1.0 - (x / div));
	}

	static float NewtonLog2(float x)
	{
		float r = x / 2; // better first guesses converge faster
		float r2 = NewtonNext(r, x);
		float delta = r - r2;
		while (delta * delta > Epsilon)
		{ 
			r = r2;
			r2 = NewtonNext(r, x);
			delta = r - r2;
		}
		return r2;
	}

	static float NewtonLog10(float x)
	{
		//static const float Log2_10 = NewtonLog2(10);
		//return NewtonLog2(x) / Log2_10;
		return 0;
	}

	//! @brief returns side of line defined by points p1, p2 that point p lies on.
	//! @return float side > 0 if on the left, side < 0 if on the right, 0 if on the line.
	static float Side(vector p1, vector p2, vector p)
	{
		return (p2[0] - p1[0]) * (p[2] - p1[2]) - (p2[2] - p1[2]) * (p[0] - p1[0]);
	}

	//! @brief Test if a + b would overflow
	//! @note despite naming, we test for both over- and underflow
	static bool TestAdditionOverflow(int a, int b)
	{
		return ((b > 0 && a > int.MAX - b) || (b < 0 && a < int.MIN - b));
	}

	//! @brief Test if a - b would overflow
	//! @note despite naming, we test for both over- and underflow
	static bool TestSubtractionOverflow(int a, int b)
	{
		return ((b < 0 && a > int.MAX + b) || (b > 0 && a < int.MIN + b));
	}

	//! @brief Test if a * b would overflow
	//! @note despite naming, we test for both over- and underflow
	static bool TestMultiplicationOverflow(int a, int b)
	{
		if ((a == -1 && b == int.MIN) || (b == -1 && a == int.MIN))
			return true;

		if ((b != 0 && a > (int) int.MAX / b) || (b != 0 && a < (int) int.MIN / b))
			return true;

		return false;
	}

	static vector Direction2D(vector p1, vector p2)
	{
		p1[1] = p2[1];
		return vector.Direction(p1, p2);
	}

	static float Distance2D(vector p1, vector p2)
	{
		p1[1] = p2[1];
		return vector.Distance(p1, p2);
	}

	static float Distance2DSq(vector p1, vector p2)
	{
		p1[1] = p2[1];
		return vector.DistanceSq(p1, p2);
	}

	static float FindLargestDistance(TVectorArray positions)
	{
		float maxDistanceSq;
		for (int i = 0; i < positions.Count() - 1; i++)
		{
			for (int j = i + 1; j < positions.Count(); j++)
			{
				float distanceSq = vector.DistanceSq(positions[i], positions[j]);
				if (distanceSq > maxDistanceSq)
					maxDistanceSq = distanceSq;
			}
		}

		return Math.Sqrt(maxDistanceSq);
	}

	static float FindShortesDistance(TVectorArray positions)
	{
		float minDistanceSq;
		for (int i = 0; i < positions.Count() - 1; i++)
		{
			for (int j = i + 1; j < positions.Count(); j++)
			{
				float distanceSq = vector.DistanceSq(positions[i], positions[j]);
				if (distanceSq < minDistanceSq)
					minDistanceSq = distanceSq;
			}
		}

		return Math.Sqrt(minDistanceSq);
	}

	/**
	 * @brief Find closest point outside cylinder that intersects the path defined by points start and end
	 * 
	 * @param start
	 * @param [inout] end  will be altered in-place if path intersects cylinder
	 * @param perpendicularDistance  positive = left, negative = right
	 * 
	 * @return true if path intersects cylinder, else false
	 */
	static bool FindClosestPointOutsideCylinder(vector start, inout vector end, vector center, float radius, float height, float perpendicularDistance = 15.0)
	{
		if (ExpansionMath.IsPointInCylinder(center, radius, height, start) || Math3D.IntersectRayCylinder(start, end, center, radius, height))
		{
			end = GetClosestPointOutsideRadius(start, end, center, radius, perpendicularDistance);

			return true;
		}

		return false;
	}

	static vector GetClosestPointOutsideRadius(vector start, vector end, vector center, float radius, float perpendicularDistance = 15.0)
	{
		//! Calculate a perpendicular direction to avoid the cylinder
		vector toCenter = center - start;
		toCenter.Normalize();
		//vector avoidanceDirectionLeft = toCenter.Perpend();
		//vector avoidanceDirectionRight = toCenter * -1.0;
		vector avoidanceDirection = toCenter.Perpend() * perpendicularDistance;

		//! Adjust the endpoint away from the edge
		vector outside = center - toCenter * radius;
		//float distToEndLeftSq = vector.DistanceSq(outside + avoidanceDirectionLeft, end);
		//float distToEndRightSq = vector.DistanceSq(outside + avoidanceDirectionRight, end);
		//vector avoidanceDirection;
		//if (distToEndLeftSq < distToEndRightSq)
			//avoidanceDirection = avoidanceDirectionLeft;
		//else
			//avoidanceDirection = avoidanceDirectionRight;

		return outside + avoidanceDirection;
	}

	static bool IsPointInCylinder(vector center, float radius, float height, vector point)
	{
		float halfHeight = height * 0.5;

		if (point[1] < center[1] - halfHeight || point[1] > center[1] + halfHeight)
			return false;

		return Math.IsPointInCircle(center, radius, point);
	}

	static float LookUp(float value, int num, float inVals[], float outVals[])
	{
		value = Math.Clamp(value, inVals[0], inVals[num - 1]);

		for (int i = 0; i < num - 1; i++)
		{
			if (inVals[i] <= value && value <= inVals[i + 1])
			{
				float delta = (value - inVals[i]) / (inVals[i + 1] - inVals[i]);
				return Math.Lerp(outVals[i], outVals[i + 1], delta);
			}
		}

		//! Should never reach this point, just here to make code compile as we need a return value
		return 0;
	}
}
