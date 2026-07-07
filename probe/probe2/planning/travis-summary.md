This is a human-authored document and should not be edited by LLM agents.

This document serves to represent Travis's mental model of the project in his own words, written by his own hand.

Glossary:

- Data:
    - The fundamental medium. Information itself. The only thing; everything else is just a kind of data.
    - Data may come by *testimony* (such as a recording, authored text, or hand-written program) or by *derivation*
- Link:
    - Data that refers to data
    - Data within the system may be pinned in a registry (equivalent, committed in a repository, memorized), which is persistent, or held in working memory, which is ephemeral
    - Data may be within process (accessible via pointer), local (accessible without network access, e.g. filesystem, ramfs, Unix pipe, locally bound UDP/TCP ports), or remote (accessible only over a network)
    - Data within process can be linked to via a pointer; if it is pinned, the pointer presumably points to a cache, unless exotic hardware is in use that enables pointer access to a persistent memory storage medium
    - Data that is pinned should be addressable by a content hash (IPFS/nix store style); a mechanism should be provided that resolves addresses to local data, performing the necessary fetching in case the data is not already accessible within process #req:content-addressable-pinned-data
    - Data may also be referred to by a *route*, which is a URL-like sequence of identifiers that can be resolved to a datum somewhere somehow #req:route-addressable-mutable-data; whereas content-addressed hashes are immutable, a route is a mutable reference that may point to different data every time it is resolved
- Node:
    - Just a bag of links to data.
    - By convention, one of the links is to the "kind" of node, which gives enough information that anyone with a compatible decoder should be able to make sense of the node.
