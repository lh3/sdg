### Side graph

Informally, for a base in a DNA sequence, a *side* is either the 5'-end or the
3'-end of the base. Each base has two sides. When we join sides in a set of
sequences with edges, we get a *side graph*. A side graph is (implicitly)
bidirected, encoding both strands of DNA sequences. It is equivalent to a
generic bidirected/string graph with a coordinate system invariable to the
addition of new sequences.

### Graph format and examples

This repo explores the storage and exchange of side graphs. For now, it only has
one functionality "reformat": read a side graph into RAM and write the graph
out. It takes a format like the following:
```
S	R1	    100       # reference R1
J	R1:20:3 R1:23:5   # a deletion
S	R2	    120
J	R2:30:5 R1:50:3   # translocation
S	A       2         # an MNP n R1
J	A:0:5   R1:10:3
J	A:1:3   R1:12:5
S	B       2
J	B:0:5   A:1:3
J	B:1:3   R1:14:5
```
where a "Sequence" line gives the sequence name and length and a "Join" line
gives the two sides that are joined together. In the input, a "J" line may use a
sequence that has not been defined on an "S" line. In the output, a "J" line
never uses sequences that have not appeared on "S" lines before.

The reformat tool optionally takes a graph in the "insert" representation like:
```
S	R1      100
S	R2      120
J	R1:50:3 R2:30:5
J	R1:20:3 R1:23:5
I	A       2        R1:10:3  R1:12:5
I	B       2        A:1:3    R1:14:5
```
The reformat of this graph is the same as the first example.

### Performance

So far, the reformat tool has only been tested on toy examples. It probably has
bugs that may show up on other graphs. Nonetheless, the underlying
implementation is very efficient and should work with huge real-world graphs in
principle.
