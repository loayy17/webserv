## 2025-05-14 - [CGI Memory Bottleneck]
**Learning:** The server buffers the entire request body in memory before passing it to the CGI process. For large uploads (e.g., 100MB+), this leads to high memory consumption and OOM crashes when multiple concurrent requests occur.
**Action:** Implement streaming from the client's receive buffer directly to the CGI pipe to eliminate the intermediate body copy.
