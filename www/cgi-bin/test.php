<?php
// Test PHP CGI Script
// Used for testing PHP CGI execution in webserv

header("Content-Type: text/html");
header("X-PHP-Test: success");
?>
<!DOCTYPE html>
<html>
<head><title>PHP CGI Test</title></head>
<body>
<h1>PHP CGI Test</h1>

<h2>Request Information:</h2>
<table border="1">
<tr><td><b>Request Method</b></td><td><?= $_SERVER['REQUEST_METHOD'] ?? 'N/A' ?></td></tr>
<tr><td><b>Query String</b></td><td><?= $_SERVER['QUERY_STRING'] ?? 'N/A' ?></td></tr>
<tr><td><b>Content Type</b></td><td><?= $_SERVER['CONTENT_TYPE'] ?? 'N/A' ?></td></tr>
<tr><td><b>Content Length</b></td><td><?= $_SERVER['CONTENT_LENGTH'] ?? 'N/A' ?></td></tr>
<tr><td><b>Script Name</b></td><td><?= $_SERVER['SCRIPT_NAME'] ?? 'N/A' ?></td></tr>
<tr><td><b>Server Name</b></td><td><?= $_SERVER['SERVER_NAME'] ?? 'N/A' ?></td></tr>
<tr><td><b>Server Port</b></td><td><?= $_SERVER['SERVER_PORT'] ?? 'N/A' ?></td></tr>
</table>

<?php if ($_SERVER['REQUEST_METHOD'] === 'GET' && !empty($_GET)): ?>
<h2>GET Parameters:</h2>
<pre><?= print_r($_GET, true) ?></pre>
<?php endif; ?>

<?php if ($_SERVER['REQUEST_METHOD'] === 'POST' && !empty($_POST)): ?>
<h2>POST Parameters:</h2>
<pre><?= print_r($_POST, true) ?></pre>
<?php endif; ?>

<h2>All Server Variables:</h2>
<details>
<summary>Click to expand</summary>
<pre><?= print_r($_SERVER, true) ?></pre>
</details>

</body>
</html>
