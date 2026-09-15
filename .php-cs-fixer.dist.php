<?php

// Formatting for the PHP tools under util/: PER Coding Style 3.0, named by
// version so a php-cs-fixer upgrade cannot change the style silently.
return (new PhpCsFixer\Config())
    ->setRules(['@PER-CS3x0' => true])
    ->setFinder(PhpCsFixer\Finder::create()->in(__DIR__ . '/util'));
