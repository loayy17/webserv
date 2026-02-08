<?php
echo "Content-Type: text/html\n\n";
echo "<h1>PHP CGI OK</h1>";

$body = file_get_contents("php://stdin");
echo "<pre>$body</pre>";
?>
