/* tslint:disable */
declare function require(name: string): any;
export const _ = require('lodash');
/* tslint:enable */

export const assert = (condition: boolean, message?: any) => {
  if (!condition) {
    console.error(message);
    throw new Error(message);
  }
};
