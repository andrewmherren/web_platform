#ifndef WEB_PLATFORM_FAVICON_ICO_H
#define WEB_PLATFORM_FAVICON_ICO_H

#include <Arduino.h>

const char WEB_PLATFORM_FAVICON[] PROGMEM = R"(
<svg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 32 32' width='32' height='32'>
  <!-- Background circle with gradient effect -->
  <defs>
    <radialGradient id='bg' cx='50%' cy='30%' r='70%'>
      <stop offset='0%' style='stop-color:#4a90e2;stop-opacity:1' />
      <stop offset='100%' style='stop-color:#2c3e50;stop-opacity:1' />
    </radialGradient>
    <linearGradient id='spark' x1='0%' y1='0%' x2='100%' y2='100%'>
      <stop offset='0%' style='stop-color:#ffd700;stop-opacity:1' />
      <stop offset='50%' style='stop-color:#ff8c00;stop-opacity:1' />
      <stop offset='100%' style='stop-color:#ff4500;stop-opacity:1' />
    </linearGradient>
  </defs>
  
  <circle cx='16' cy='16' r='15' fill='url(#bg)' stroke='#34495e' stroke-width='1'/>
  
  <!-- Central spark/lightning symbol -->
  <path d='M12 8 L20 14 L15 14 L20 24 L12 18 L17 18 Z' fill='url(#spark)' stroke='#ffd700' stroke-width='0.5'/>
  
  <!-- Energy particles/dots around the spark -->
  <circle cx='8' cy='10' r='1' fill='#ffd700' opacity='0.8'>
    <animate attributeName='opacity' values='0.8;0.3;0.8' dur='2s' repeatCount='indefinite'/>
  </circle>
  <circle cx='24' cy='14' r='0.8' fill='#ff8c00' opacity='0.6'>
    <animate attributeName='opacity' values='0.6;0.2;0.6' dur='2.5s' repeatCount='indefinite'/>
  </circle>
  <circle cx='6' cy='22' r='0.6' fill='#ff4500' opacity='0.7'>
    <animate attributeName='opacity' values='0.7;0.1;0.7' dur='1.8s' repeatCount='indefinite'/>
  </circle>
  <circle cx='26' cy='8' r='0.5' fill='#ffd700' opacity='0.5'>
    <animate attributeName='opacity' values='0.5;0.1;0.5' dur='3s' repeatCount='indefinite'/>
  </circle>
</svg>
)";

#endif