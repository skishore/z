type int = number;
interface point {x: int, y: int};

const assert = (x: boolean): void => {
  if (!x) throw new Error();
};

const range = (n: int): int[] => {
  const result = [];
  for (let i = 0; i < n; i++) {
    result.push(i);
  }
  return result;
};

export {assert, int, point, range};
