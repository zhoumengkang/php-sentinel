<?php
error_reporting(E_ALL);
ini_set("display_errors", true);

$br = (php_sapi_name() == "cli") ? "" : "<br>";

if (!extension_loaded('sentinel')) {
    dl('sentinel.' . PHP_SHLIB_SUFFIX);
}
$module    = 'sentinel';
$functions = get_extension_funcs($module);
echo "Functions available in the test extension:$br\n";
foreach ($functions as $func) {
    echo $func . "$br\n";
}
echo "$br\n";
$function = 'confirm_' . $module . '_compiled';
if (extension_loaded($module)) {
    $str = $function($module);
} else {
    $str = "Module $module is not compiled into PHP";
}
echo "$str\n";

class aa
{
    public static function bb()
    {
        echo 222222;
        echo "\n";
    }
}

try {
    aa::bb();
} catch (Exception $e) {
    var_dump($e);
}

function ccdd()
{
    echo 333333;
    echo "\n";
}

ccdd();