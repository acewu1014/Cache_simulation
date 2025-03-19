## Project Name: 
### The cache behavior simulation with different indexing strategies

## Strategy:
>* LSB(Least Significant Bits)
>* Optimized strategy(cite:[link](https://dl.acm.org/doi/10.1145/1124713.1124715))

## Optimized Strategy:
Calculate the correlation between each bit for every reference and derive the correlation matrix. Then, use the occurrence count of reference 0 and 1 to compute the quality score. In each round, select the highest quality score as the index. Multiply the remaining values by the correlation and select the next highest, repeating the process until a sufficient number of index bits are selected.

## Result:
In the test cases, the optimized strategy reduces the miss count by approximately 23%.