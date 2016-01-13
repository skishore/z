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
  def *(scale: Double) = Vec((x * scale).toInt, (y * scale).toInt);
  def /(scale: Double) = Vec((x / scale).toInt, (y / scale).toInt);
}

case class Rect(pos: Vec, size: Vec) {
  def left = min(pos.x, pos.x + size.x)
  def top = min(pos.y, pos.y + size.y)
  def right = max(pos.x, pos.x + size.x)
  def bottom = max(pos.y, pos.y + size.y)

  def area = abs(size.area)
  def center = Vec(pos.x + size.x / 2, pos.y + size.y / 2)
}

object Rect {
  def column(x: Int, y: Int, height: Int) = Rect(Vec(x, y), Vec(1, height))
  def row(x: Int, y: Int, width: Int) = Rect(Vec(x, y), Vec(width, 1))
}
