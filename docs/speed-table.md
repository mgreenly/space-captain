# Spacecraft Speed Table

## Overview

Space Captain uses a logarithmic speed scale from 1 to 1000, where:
- Speed 1 = 500 m/s (minimum speed)
- Speed 1000 = 1.67 × 10¹² m/s (traverse radius in 10 minutes)

## Logarithmic Formula

The speed at any dial setting n (1-1000) is calculated as:
```
speed(n) = 500 × 10^((n-1) × log₁₀(3.33×10⁹) / 999)
```

## C Implementation

```c
#include <math.h>

// Calculate speed in meters/second for a given speed dial setting (1-1000)
double sc_speed_from_dial(int dial) {
    if (dial < 1) dial = 1;
    if (dial > 1000) dial = 1000;
    
    // Base values
    const double min_speed = 500.0;                    // Speed at dial 1
    const double max_speed = 1.666666666666667e12;     // Speed at dial 1000
    const double ratio = max_speed / min_speed;        // 3.333333333333334e9
    
    // Logarithmic interpolation
    // speed = min_speed * 10^((dial-1) * log10(ratio) / 999)
    double exponent = (dial - 1) * log10(ratio) / 999.0;
    return min_speed * pow(10.0, exponent);
}
```

## Speed Table (Every 10th Value)

| Speed Dial | Meters/Second | Scientific Notation | Travel Time to Edge | Notes |
|------------|---------------|-------------------|-------------------|--------|
| 1 | 500 | 5.00 × 10² | 63,419 years | Minimum speed |
| 11 | 623 | 6.23 × 10² | 50,921 years | |
| 21 | 776 | 7.76 × 10² | 40,886 years | |
| 31 | 966 | 9.66 × 10² | 32,829 years | |
| 41 | 1,203 | 1.20 × 10³ | 26,359 years | |
| 51 | 1,498 | 1.50 × 10³ | 21,164 years | |
| 61 | 1,866 | 1.87 × 10³ | 16,993 years | |
| 71 | 2,324 | 2.32 × 10³ | 13,644 years | |
| 81 | 2,894 | 2.89 × 10³ | 10,956 years | |
| 91 | 3,605 | 3.60 × 10³ | 8,796 years | |
| 101 | 4,490 | 4.49 × 10³ | 7,063 years | |
| 111 | 5,592 | 5.59 × 10³ | 5,671 years | |
| 121 | 6,964 | 6.96 × 10³ | 4,553 years | |
| 131 | 8,673 | 8.67 × 10³ | 3,656 years | |
| 141 | 10,802 | 1.08 × 10⁴ | 2,936 years | |
| 151 | 13,453 | 1.35 × 10⁴ | 2,357 years | |
| 161 | 16,755 | 1.68 × 10⁴ | 1,893 years | |
| 171 | 20,868 | 2.09 × 10⁴ | 1,520 years | |
| 181 | 25,990 | 2.60 × 10⁴ | 1,220 years | |
| 191 | 32,369 | 3.24 × 10⁴ | 980 years | |
| 201 | 40,314 | 4.03 × 10⁴ | 787 years | |
| 211 | 50,208 | 5.02 × 10⁴ | 632 years | |
| 221 | 62,532 | 6.25 × 10⁴ | 507 years | |
| 231 | 77,880 | 7.79 × 10⁴ | 407 years | |
| 241 | 96,995 | 9.70 × 10⁴ | 327 years | |
| 251 | 120,802 | 1.21 × 10⁵ | 262 years | |
| 261 | 150,452 | 1.50 × 10⁵ | 211 years | |
| 271 | 187,379 | 1.87 × 10⁵ | 169 years | |
| 281 | 233,370 | 2.33 × 10⁵ | 136 years | |
| 291 | 290,649 | 2.91 × 10⁵ | 109 years | |
| 301 | 361,987 | 3.62 × 10⁵ | 87.6 years | |
| 311 | 450,835 | 4.51 × 10⁵ | 70.3 years | |
| 321 | 561,489 | 5.61 × 10⁵ | 56.5 years | |
| 331 | 699,303 | 6.99 × 10⁵ | 45.3 years | |
| 341 | 870,943 | 8.71 × 10⁵ | 36.4 years | |
| 351 | 1,084,711 | 1.08 × 10⁶ | 29.2 years | |
| 361 | 1,350,946 | 1.35 × 10⁶ | 23.5 years | |
| 371 | 1,682,527 | 1.68 × 10⁶ | 18.8 years | |
| 381 | 2,095,493 | 2.10 × 10⁶ | 15.1 years | |
| 391 | 2,609,819 | 2.61 × 10⁶ | 12.2 years | |
| 401 | 3,250,382 | 3.25 × 10⁶ | 9.8 years | |
| 411 | 4,048,168 | 4.05 × 10⁶ | 7.8 years | |
| 421 | 5,041,766 | 5.04 × 10⁶ | 6.3 years | |
| 431 | 6,279,236 | 6.28 × 10⁶ | 5.0 years | |
| 441 | 7,820,435 | 7.82 × 10⁶ | 4.1 years | |
| 451 | 9,739,912 | 9.74 × 10⁶ | 3.3 years | |
| 461 | 12,130,512 | 1.21 × 10⁷ | 2.6 years | |
| 471 | 15,107,870 | 1.51 × 10⁷ | 2.1 years | |
| 481 | 18,816,002 | 1.88 × 10⁷ | 1.7 years | |
| 491 | 23,434,272 | 2.34 × 10⁷ | 1.4 years | |
| 501 | 29,186,068 | 2.92 × 10⁷ | 396 days | |
| 511 | 36,349,605 | 3.63 × 10⁷ | 318 days | |
| 521 | 45,271,388 | 4.53 × 10⁷ | 256 days | |
| 531 | 56,382,966 | 5.64 × 10⁷ | 205 days | |
| 541 | 70,221,812 | 7.02 × 10⁷ | 165 days | |
| 551 | 87,457,315 | 8.75 × 10⁷ | 132 days | |
| 561 | 108,923,165 | 1.09 × 10⁸ | 106 days | |
| 571 | 135,657,673 | 1.36 × 10⁸ | 85.3 days | |
| 581 | 168,953,999 | 1.69 × 10⁸ | 68.5 days | |
| 591 | 210,422,699 | 2.10 × 10⁸ | 55.0 days | |
| 601 | 262,069,632 | 2.62 × 10⁸ | 44.2 days | |
| 611 | 326,392,980 | 3.26 × 10⁸ | 35.5 days | ~1c |
| 621 | 406,504,092 | 4.07 × 10⁸ | 28.5 days | ~1c |
| 631 | 506,277,973 | 5.06 × 10⁸ | 22.9 days | ~2c |
| 641 | 630,540,729 | 6.31 × 10⁸ | 18.4 days | ~2c |
| 651 | 785,302,999 | 7.85 × 10⁸ | 14.7 days | ~3c |
| 661 | 978,050,699 | 9.78 × 10⁸ | 11.8 days | ~3c |
| 671 | 1,218,107,113 | 1.22 × 10⁹ | 9.5 days | ~4c |
| 681 | 1,517,083,870 | 1.52 × 10⁹ | 7.6 days | ~5c |
| 691 | 1,889,442,597 | 1.89 × 10⁹ | 6.1 days | ~6c |
| 701 | 2,353,194,441 | 2.35 × 10⁹ | 4.9 days | ~8c |
| 711 | 2,930,771,269 | 2.93 × 10⁹ | 3.9 days | ~10c |
| 721 | 3,650,110,709 | 3.65 × 10⁹ | 3.2 days | ~12c |
| 731 | 4,546,007,507 | 4.55 × 10⁹ | 2.5 days | ~15c |
| 741 | 5,661,796,558 | 5.66 × 10⁹ | 2.0 days | ~19c |
| 751 | 7,051,449,040 | 7.05 × 10⁹ | 1.6 days | ~24c |
| 761 | 8,782,183,014 | 8.78 × 10⁹ | 1.3 days | ~29c |
| 771 | 10,937,714,795 | 1.09 × 10¹⁰ | 1.1 days | ~36c |
| 781 | 13,622,308,342 | 1.36 × 10¹⁰ | 20.4 hours | ~45c |
| 791 | 16,965,818,551 | 1.70 × 10¹⁰ | 16.4 hours | ~57c |
| 801 | 21,129,972,386 | 2.11 × 10¹⁰ | 13.1 hours | ~70c |
| 811 | 26,316,191,682 | 2.63 × 10¹⁰ | 10.6 hours | ~88c |
| 821 | 32,775,336,001 | 3.28 × 10¹⁰ | 8.5 hours | ~109c |
| 831 | 40,819,836,812 | 4.08 × 10¹⁰ | 6.8 hours | ~136c |
| 841 | 50,838,809,931 | 5.08 × 10¹⁰ | 5.5 hours | ~169c |
| 851 | 63,316,877,211 | 6.33 × 10¹⁰ | 4.4 hours | ~211c |
| 861 | 78,857,607,901 | 7.89 × 10¹⁰ | 3.5 hours | ~263c |
| 871 | 98,212,713,543 | 9.82 × 10¹⁰ | 2.8 hours | ~327c |
| 881 | 122,318,408,562 | 1.22 × 10¹¹ | 2.3 hours | ~408c |
| 891 | 152,340,695,347 | 1.52 × 10¹¹ | 1.8 hours | ~508c |
| 901 | 189,731,764,268 | 1.90 × 10¹¹ | 1.5 hours | ~632c |
| 911 | 236,300,236,717 | 2.36 × 10¹¹ | 1.2 hours | ~788c |
| 921 | 294,298,648,875 | 2.94 × 10¹¹ | 56.6 minutes | ~981c |
| 931 | 366,532,407,807 | 3.67 × 10¹¹ | 45.5 minutes | ~1222c |
| 941 | 456,495,490,166 | 4.56 × 10¹¹ | 36.5 minutes | ~1522c |
| 951 | 568,539,447,271 | 5.69 × 10¹¹ | 29.3 minutes | ~1895c |
| 961 | 708,083,891,443 | 7.08 × 10¹¹ | 23.5 minutes | ~2360c |
| 971 | 881,878,644,883 | 8.82 × 10¹¹ | 18.9 minutes | ~2940c |
| 981 | 1,098,330,231,344 | 1.10 × 10¹² | 15.2 minutes | ~3661c |
| 991 | 1,367,908,503,154 | 1.37 × 10¹² | 12.2 minutes | ~4560c |
| 1000 | 1,666,666,666,667 | 1.67 × 10¹² | 10.0 minutes | Maximum speed |

## Gameplay Implications

- **Speeds 1-300**: Suitable for local maneuvering and combat
- **Speeds 301-600**: Interplanetary travel within inner solar system
- **Speeds 601-800**: Rapid transit across solar system
- **Speeds 801-1000**: Emergency/strategic repositioning

Note: 'c' denotes the speed of light (3 × 10⁸ m/s). Many speeds exceed this physical limit, representing game mechanics rather than realistic physics.

## Warp Speed Comparison

For reference, here's how Space Captain speeds compare to Star Trek warp factors (using TNG scale):

| Warp Factor | Speed (× c) | Space Captain Equivalent |
|-------------|-------------|-------------------------|
| Warp 1 | 1c | Speed ~611 |
| Warp 2 | 10c | Speed ~711 |
| Warp 3 | 39c | Speed ~757 |
| Warp 4 | 102c | Speed ~807 |
| Warp 5 | 214c | Speed ~841 |
| Warp 6 | 392c | Speed ~869 |
| Warp 7 | 656c | Speed ~893 |
| Warp 8 | 1,024c | Speed ~911 |
| Warp 9 | 1,516c | Speed ~927 |
| Warp 9.9 | 3,053c | Speed ~951 |
| Warp 9.99 | 7,912c | Speed ~976 |

Space Captain's maximum speed (Speed 1000 at ~5,556c) falls between Warp 9.9 and Warp 9.99, making our fastest ships comparable to the most advanced vessels in Star Trek.

## Interstellar Travel Example

At maximum speed (Speed 1000), the journey from Sol to Alpha Centauri would take:
- Distance: 4.37 light-years
- Speed: 1.67 × 10¹² m/s (~5,556c)
- **Travel time: ~6.9 hours**

For comparison, at the speed of light this journey would take 4.37 years.