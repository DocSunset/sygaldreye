atom.construct v0.0.0

Mint-or-get the primitive (leaf) type named by `name` with the given instance
size and alignment.

Inputs:     name: id (a str or scope node), size: u64, align: u64
Outputs:    out: id (the atom type node)
Transition: Form the atom_term {name, size, align} in the substrate's fixed
            layout and take its node under type `atom`. If that id is already
            resident, return it (its bytes verified byte-identical); else
            enroll and return it. Idempotent — equal inputs yield the same id
            on every peer.
Faults:     id present with distinct bytes ⇒ hard error, never a silent merge.
