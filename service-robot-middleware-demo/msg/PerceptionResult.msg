std_msgs/Header header

# Detected objects
ObjectInfo[] objects

# Scene classification
string scene_label
float64 scene_confidence

# Image processing metadata
uint32 image_width
uint32 image_height
float64 processing_latency_ms

# Region of interest
geometry_msgs/Point[] roi_points
