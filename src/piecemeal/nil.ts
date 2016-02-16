// Support for a [Nil] type that allows for proper maybe-typed interfaces.
// For example, you can write a function that returns string|Nil and the
// compiler will check that callees handle both cases.
export class Nil {}
export const nil = new Nil;
