<html>
<head>
<title>Hello PHP CGI</title>
</head>
<body>
<?php
$first_name = htmlspecialchars($_POST['fname']);
$last_name = htmlspecialchars($_POST['lname']);
echo "<h2>Hello " . $first_name . " " . $last_name . "</h2>";
?>
</body>
</html>
