idf_component_register(
    SRCS "yolo11_pose.cpp"
    INCLUDE_DIRS "."
    REQUIRES coco_pose esp-dl
)
