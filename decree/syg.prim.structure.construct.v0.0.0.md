structure.construct v0.0.0

Mint-or-get the product type: a name followed by ordered (field-name,
field-type) pairs.

Inputs:     name: id (str/scope, or GROUND for anonymous), then zero or more
            (field-name: id, field-type: id) pairs
Outputs:    out: id (the structure type node)
Transition: Form the term [name][field_term...] in declaration order and take
            its node under type `structure`. Arity is recovered from the node's
            size, never stored. name = GROUND ⇒ anonymous — two anonymous
            structures with identical fields share one id. Mint-or-get with
            verified dedup, as atom.construct.
Faults:     id present with distinct bytes ⇒ hard error.
