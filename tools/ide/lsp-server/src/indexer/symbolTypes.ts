export interface Range {
  start: { line: number; character: number };
  end: { line: number; character: number };
}

export interface Symbol {
  name: string;
  kind: SymbolKind;
  location: {
    uri: string;
    /** the name */
    range: Range;
  };
  isDefinition: boolean;
  scope?: 'local' | 'global' | 'extern';
  /** the declaration as written (one line), for hover and completion */
  detail?: string;
  /** from start to end (a function's body, a struct's members): the outline */
  fullRange?: Range;
  /** members, enumerators, a proc's labels */
  children?: Symbol[];
  /** a header of the compiler's library (lib/include), not the workspace's */
  library?: boolean;
}

/** A C type as declared: the base (int, struct node, node_t) and the pointer level */
export interface CType {
  base: string;
  ptr: number;
}

export interface MemberInfo extends CType {
  name: string;
  method: boolean;
  decl: string;
  line: number;
  character: number;
}

export interface AggregateInfo {
  /** the tag (struct node), or for an anonymous one the typedef's name */
  tag: string;
  kind: 'struct' | 'union';
  members: MemberInfo[];
  uri: string;
  line: number;
}

/** What a C scan finds besides the symbols: types and the variables' types */
export interface CTypes {
  aggregates: AggregateInfo[];
  typedefs: Map<string, CType>;
  /** file-scope variables */
  vars: Map<string, CType>;
  /** #define bodies (vic4 → ((volatile struct vic4_regs *)0xD000)) */
  macros: Map<string, string>;
}

export enum SymbolKind {
  File = 1,
  Module = 2,
  Namespace = 3,
  Package = 4,
  Class = 5,
  Method = 6,
  Property = 7,
  Field = 8,
  Constructor = 9,
  Enum = 10,
  Interface = 11,
  Function = 12,
  Variable = 13,
  Constant = 14,
  String = 15,
  Number = 16,
  Boolean = 17,
  Array = 18,
  Object = 19,
  Key = 20,
  Null = 21,
  EnumMember = 22,
  Struct = 23,
  Event = 24,
  Operator = 25,
  TypeParameter = 26,
}

export interface IndexedFile {
  uri: string;
  symbols: Symbol[];
  timestamp: number;
  /** C files: the types (member completion) */
  types?: CTypes;
  /** the identifiers in the file (references: the files worth reading) */
  words?: Set<string>;
  library?: boolean;
}
