All-Band High-Precision GNSS Antenna – u-blox ANN-MB2-00

The L1/L2/L5/E6/B3/L all-band high-precision GNSS antenna (ANN-MB2-00) is a professional-grade, multi-band active GNSS antenna designed by u-blox for centimeter-level positioning accuracy. It is commonly used with u-blox high-precision receivers such as the EVK-X20P-00 and is targeted at demanding applications including robotics, surveying, autonomous systems, industrial IoT, and research platforms.

The “-00” variant denotes the standard commercial version, typically supplied with a 5-meter RF cable and SMA connector, allowing easy plug-and-play integration with evaluation kits and embedded GNSS modules.

This antenna supports multi-constellation and multi-frequency reception, enabling robust positioning even in challenging environments. It simultaneously receives signals from GPS (USA), GLONASS (Russia), Galileo (Europe), BeiDou (China), and NavIC (India) across several frequency bands, significantly improving accuracy, availability, and reliability.

Key Capabilities of ANN-MB2-00

The antenna is designed for high-precision positioning techniques, including:

• Real-Time Kinematic (RTK) – enables centimeter-level real-time positioning
• Precise Point Positioning (PPP) – provides high accuracy without a local base station
• All-band reception – supports L1, L2, L5, E6, B3, and L bands
• Multi-constellation tracking – improves satellite geometry and reduces signal outages

Because it supports multiple frequencies, it can correct ionospheric errors more effectively than single-band antennas, which is critical for professional GNSS applications.

GNSS Monitoring Application with EVK-X20P-00

The GNSS monitoring application developed for the u-blox EVK-X20P-00 successfully decodes and visualizes real-time NMEA-0183 data received through a serial interface. The application continuously processes incoming GNSS messages and displays meaningful navigation, timing, motion, and satellite-health information.

Positioning Information

The application extracts core positioning data primarily from the $GNGGA and $GNRMC NMEA sentences. These provide:

• Latitude – current geographic latitude with hemisphere (North or South)
• Longitude – current geographic longitude with hemisphere (East or West)
• Altitude (MSL) – height above mean sea level in meters
• Geoid Separation – difference between the WGS-84 ellipsoid and mean sea level
• RTK correction status – indicates whether RTK fix, float, or standalone mode is active

This information represents the real-time spatial position of the GNSS receiver.

Timing Information

Accurate timing data is obtained from $GNRMC and $GNZDA messages. The system maintains:

• UTC time derived directly from GNSS satellites
• Current UTC date

This GNSS-derived time is highly precise and suitable for synchronization, logging, and timestamping applications.

Motion and Velocity Information

Motion-related parameters are decoded from $GNVTG and $GNRMC sentences, including:

• Ground speed in knots
• Converted ground speed in kilometers per hour (km/h)
• True course (heading relative to true north)
• Magnetic course (heading relative to magnetic north)

These values are essential for navigation, vehicle tracking, and dynamic motion analysis.

Accuracy and Fix Quality Assessment

Fix quality and accuracy metrics are derived from $GNGSA and $GNGGA sentences. The application reports:

• Fix type – confirms whether a valid 3D position fix (latitude, longitude, altitude) is available
• Number of satellites used in the navigation solution
• PDOP (Position Dilution of Precision) – overall positional accuracy indicator
• HDOP (Horizontal Dilution of Precision) – horizontal accuracy measure
• VDOP (Vertical Dilution of Precision) – vertical accuracy measure

Lower DOP values indicate better satellite geometry and higher positional accuracy.

Satellite Health and Visibility Monitoring

Satellite-level diagnostics are extracted from $GPGSV and $GNGSV messages. For each visible satellite, the application displays:

• Satellite PRN (ID)
• GNSS constellation (GPS, Galileo, BeiDou, etc.)
• Elevation angle in degrees
• Azimuth angle in degrees
• Signal-to-Noise Ratio (SNR) in dB-Hz

This allows real-time assessment of satellite signal quality and environmental effects such as obstructions or interference.

Raw NMEA Data Stream

The system also displays the raw, unprocessed NMEA-0183 sentences for debugging and validation purposes. These include:

• $GNGGA – position and altitude
• $GNRMC – time, date, speed, and course
• $GNGSA – fix type and DOP values
• $GPGSV / $GNGSV – satellite visibility
• $GNVTG – velocity and heading

This raw data is useful for protocol analysis, firmware testing, and research work.

Date and Time Logging

The application maintains a continuous UTC date and time log, which is used for:

• Data validation
• Post-processing and analysis
• Event correlation with external systems

This feature is especially important in high-precision GNSS experiments and long-term data collection.

Example: GPS Satellite Visibility Message Analysis

Consider the following NMEA sentence:

$GPGSV,3,1,11,01,09,155,22,02,14,125,17,03,10,180,10,04,57,144,21,1*66


The *last number before the checksum (66) is the Signal ID, which in this case is 1.

This indicates:
• System: GP (GPS – United States)
• Signal ID: 1
• Signal Type: L1 C/A
• Frequency: 1575.42 MHz

This is the standard civilian GPS signal used by most consumer GNSS devices.

Satellite Performance Interpretation

From this message, four GPS satellites are visible, but the signal quality is relatively weak:

• A satellite at 57° elevation shows an SNR of 21 dB-Hz, which is unusually low for such a high elevation and may indicate partial obstruction (trees, buildings, antenna placement issues).
• Satellites at low elevation angles (9°–14°) show weak SNR values, which is expected due to atmospheric attenuation and horizon effects.
• A satellite with 10 dB-Hz SNR is effectively unusable and contributes little to positioning accuracy.

Master List of GNSS Signal IDs (All Systems)
GPS (System ID: GP – USA)

• Signal ID 1 → L1 C/A at 1575.42 MHz (standard civilian)
• Signal ID 5 → L2 CM at 1227.60 MHz (surveying/military)
• Signal ID 6 → L2 CL at 1227.60 MHz
• Signal ID 8 → L5 at 1176.45 MHz (high precision, safety-of-life)

BeiDou (System ID: GB – China)

• Signal ID 1 → B1I at 1561.09 MHz
• Signal ID 3 → B1C at 1575.42 MHz (GPS compatible)
• Signal ID 5 → B2a at 1176.45 MHz
• Signal ID 7 → B2b at 1207.14 MHz
• Signal ID 8 → B3I at 1268.52 MHz (high reliability)

Galileo (System ID: GA – Europe)

• Signal ID 7 → E1 at 1575.42 MHz
• Signal ID 2 → E5a at 1176.45 MHz
• Signal ID 8 → E5b at 1207.14 MHz
• Signal ID 3 → E6 at 1278.75 MHz (High Accuracy Service – HAS)

GLONASS (System ID: GL – Russia)

• Signal ID 1 → L1OF at ~1602 MHz
• Signal ID 3 → L2OF at ~1246 MHz

NavIC (System ID: GI – India)

• Signal ID 1 → L5 at 1176.45 MHz (primary regional navigation signal)
