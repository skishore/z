package piecemeal

import scala.math.{abs, max, min, sqrt}

case class Rect(pos: Vec, size: Vec) extends Iterable[Vec] {
  def left = min(pos.x, pos.x + size.x)
  def top = min(pos.y, pos.y + size.y)
  def right = max(pos.x, pos.x + size.x)
  def bottom = max(pos.y, pos.y + size.y)

  def area = abs(size.area)
  def center = Vec(pos.x + size.x / 2, pos.y + size.y / 2)

  def topLeft = Vec(left, top)
  def topRight = Vec(right, top)
  def bottomLeft = Vec(left, bottom)
  def bottomRight = Vec(right, bottom)

  // Returns a new rectangle that is the intersection of [a] and [b].
  def intersect(other: Rect) {
    val left = max(left, other.left)
    val right = min(right, other.right)
    val top = max(top, other.top)
    val bottom = min(bottom, other.bottom)

    val width = max(0, right - left);
    val height = max(0, bottom - top);
    return Rect(Vec(left, top), Vec(width, height));
  }

  // Returns a [Vec] within the [Rect] that is as close to [point] as possible.
  def clamp(point: Vec) {
    val x = max(left, min(point.x, right - 1));
    val y = max(top, min(point.x, bottom - 1));
    return Vec(x, y);
  }

  def contains(point: Vec) {
    if (point.x < left) return false;
    if (point.x >= right) return false;
    if (point.y < top) return false;
    if (point.y >= bottom) return false;
    return true;
  }

  def containsRect(other: Rect) {
    if (other.left < left) return false;
    if (other.right > right) return false;
    if (other.top < top) return false;
    if (other.bottom > bottom) return false;
    return true;
  }

  // Returns the distance between Rect and [other]. This is minimum
  // length that a corridor would have to be to go from one Rect to the other.
  // If the two Rects are adjacent, returns zero. If they overlap, returns -1.
  def distance(other: Rect) {
    var vertical = -1;
    if (top >= other.bottom) {
      vertical = top - other.bottom;
    } else if (bottom <= other.top) {
      vertical = other.top - bottom;
    }

    var horizontal = -1;
    if (left >= other.right) {
      horizontal = left - other.right;
    } else if (right <= other.left) {
      horizontal = other.left - right;
    }

    if ((vertical === -1) && (horizontal === -1)) return -1;
    if (vertical === -1) return horizontal;
    if (horizontal === -1) return vertical;
    return horizontal + vertical;
  }

  def inflate(distance: Int) {
    return new Rect(Vec(left - distance, top - distance),
                    Vec(width + (distance * 2), height + (distance * 2)));
  }

  // Iterates over the points along the edge of the [Rect].
  def trace() {
    if (width > 1 && height > 1) {
      return Range(left, right).flatMap(List(Vec(_, top), Vec(_, bottom - 1)))
      return Range(top, bottom).flatMap(List(Vec(left, _), Vec(right - 1, _)))
    } else {
      for (x <- this.iterator()) yield x;
    }
  }

  // Iterates over all points within this [Rect].
  def iterator() {
    let x = left;
    let y = top;
    if (width === 0) return;
    while (y < bottom) {
      yield new Vec(x, y);
      x += 1;
      if (x >= right) {
        x = left;
        y += 1;
      }
    }
  }
}

object Rect {
  def column(x: Int, y: Int, height: Int) = Rect(Vec(x, y), Vec(1, height))
  def row(x: Int, y: Int, width: Int) = Rect(Vec(x, y), Vec(width, 1))
}
