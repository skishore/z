package piecemeal

import scala.math.{abs, max, min, sqrt}

// A class representing a 2-dimensional vector. Provides arithmetic operations
// and a base classe for Direction.
case class Vec(x: Int, y: Int) {
  // Returns the signed area of the rectangle from (0, 0) to this Vec.
  def area = x * y

  // The Euclidean norm of this Vec.
  def length = sqrt(lengthSquared.toDouble)
  def lengthSquared = x * x + y * y

  // Other norms of this Vec.
  def rookLength = abs(x) + abs(y)
  def kingLength = max(abs(x), abs(y))

  // Arithmetic operations between two Vecs or between a Vec and an number.
  def +(other: Vec) = Vec(x + other.x, y + other.y)
  def -(other: Vec) = Vec(x - other.x, y - other.y)
  def *(scale: Double) = Vec((x * scale).toInt, (y * scale).toInt)
  def /(scale: Double) = Vec((x / scale).toInt, (y / scale).toInt)
}
