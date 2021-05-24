<?php
error_reporting(E_ALL);
ini_set("display_errors", true);

class aa
{
    public static function bb()
    {
        echo 222222;
        echo "\n";
    }
}

function ccdd()
{
    echo 333333;
    echo "\n";
}

ccdd();
aa::bb();
ccdd();