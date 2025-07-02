# Spacecraft Speed Table

## Overview

Space Captain uses a logarithmic speed scale from 1 to 100, where:
- Speed 1 = 500 m/s (minimum speed)
- Speed 100 = 1.67 × 10¹² m/s (traverse radius in 10 minutes)

## Logarithmic Formula

The speed at any dial setting n (1-100) is calculated as:
```
speed(n) = 500 × 10^((n-1) × log₁₀(3.33×10⁹) / 99)
```

## Speed Table (Every 5th Value)

| Speed Dial | Meters/Second | Scientific Notation | Travel Time to Edge | Notes |
|------------|---------------|-------------------|-------------------|--------|
| 1 | 500 | 5.00 × 10² | 63,419 years | Minimum speed |
| 5 | 2,969 | 2.97 × 10³ | 10,675 years | |
| 10 | 21,041 | 2.10 × 10⁴ | 1,506 years | |
| 15 | 149,164 | 1.49 × 10⁵ | 212 years | |
| 20 | 1,057,402 | 1.06 × 10⁶ | 30.0 years | |
| 25 | 7,496,404 | 7.50 × 10⁶ | 4.23 years | |
| 30 | 53,133,103 | 5.31 × 10⁷ | 217 days | |
| 35 | 376,634,277 | 3.77 × 10⁸ | 30.7 days | ~1.26c |
| 40 | 2,669,567,478 | 2.67 × 10⁹ | 4.33 days | ~8.9c |
| 45 | 18,919,840,634 | 1.89 × 10¹⁰ | 14.7 hours | |
| 50 | 134,087,260,468 | 1.34 × 10¹¹ | 2.07 hours | |
| 55 | 950,408,491,994 | 9.50 × 10¹¹ | 17.5 minutes | |
| 60 | 6,736,185,383,900 | 6.74 × 10¹² | 2.47 minutes | |
| 65 | 47,746,421,287,526 | 4.77 × 10¹³ | 20.9 seconds | |
| 70 | 338,474,001,286,590 | 3.38 × 10¹⁴ | 2.95 seconds | |
| 75 | 2,399,402,093,401,840 | 2.40 × 10¹⁵ | 0.417 seconds | |
| 80 | 17,006,802,394,376,700 | 1.70 × 10¹⁶ | 0.0588 seconds | |
| 85 | 120,559,517,323,767,000 | 1.21 × 10¹⁷ | 0.0083 seconds | |
| 90 | 854,615,144,214,085,000 | 8.55 × 10¹⁷ | 0.00117 seconds | |
| 95 | 6,058,893,248,803,330,000 | 6.06 × 10¹⁸ | 0.000165 seconds | |
| 100 | 1,666,666,666,666,670,000 | 1.67 × 10¹⁸ | 10 minutes | Maximum speed |

## Gameplay Implications

- **Speeds 1-30**: Suitable for local maneuvering and combat
- **Speeds 31-60**: Interplanetary travel within inner solar system
- **Speeds 61-80**: Rapid transit across solar system
- **Speeds 81-100**: Emergency/strategic repositioning

Note: 'c' denotes the speed of light (3 × 10⁸ m/s). Many speeds exceed this physical limit, representing game mechanics rather than realistic physics.