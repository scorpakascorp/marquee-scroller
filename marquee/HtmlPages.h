#pragma once

#include <Arduino.h>

static const char MAIN_PAGE_HTML[] PROGMEM = R"=====(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <link rel = "stylesheet" href = "https://www.w3schools.com/w3css/4/w3.css">
  <link rel = "stylesheet" href = "https://www.w3schools.com/lib/w3-theme-blue-grey.css">
  <link rel = "stylesheet" href = "https://cdnjs.cloudflare.com/ajax/libs/font-awesome/5.8.1/css/all.min.css">
</head>
<body>
  <nav class="w3-sidebar w3-bar-block w3-card" style="margin-top:88px" id="mySidebar">
    <div class="w3-container w3-theme-d2">
      <span onclick="closeSidebar()" class="w3-button w3-display-topright w3-large">
        <i class="fas fa-times"></i>
      </span>
      <div class="w3-padding">Menu</div>
    </div>
)=====";