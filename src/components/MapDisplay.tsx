import React, { useState, useEffect, useRef } from 'react';
import { View, Image, StyleSheet, Text, LayoutChangeEvent } from 'react-native';
import Svg, { Polygon, G, Circle, Text as SvgText, Line } from 'react-native-svg';

export interface MapMarker {
    x: number; // world coordinates
    y: number; // world coordinates
    color?: string;
    label?: string;
}

interface MapDisplayProps {
    mapImage: string | null;
    resolution: number;
    originX: number;
    originY: number;
    mapHeight: number;
    mapWidth: number;
    robotPose: { x: number; y: number; theta: number } | null;
    rotation?: number; // 0, 90, 180, 270
    customMarkers?: MapMarker[];
    destination?: { x: number; y: number } | null;
    onMapPress?: (worldX: number, worldY: number) => void;
}

const MapDisplay: React.FC<MapDisplayProps> = ({
    mapImage,
    resolution,
    originX,
    originY,
    mapHeight,
    mapWidth,
    robotPose,
    rotation = 0,
    customMarkers = [],
    destination,
    onMapPress
}) => {
    const [imageSize, setImageSize] = useState({ width: 0, height: 0 });
    const [containerSize, setContainerSize] = useState({ width: 0, height: 0 });
    const containerRef = useRef<View>(null);

    useEffect(() => {
        if (mapImage) {
            Image.getSize(mapImage, (width, height) => {
                setImageSize({ width, height });
            }, (error) => {
                console.error("Failed to get image size", error);
            });
        }
    }, [mapImage]);

    const onLayout = (event: LayoutChangeEvent) => {
        const { width, height } = event.nativeEvent.layout;
        const roundedWidth = Math.round(width);
        const roundedHeight = Math.round(height);

        if (roundedWidth !== containerSize.width || roundedHeight !== containerSize.height) {
            setContainerSize({ width: roundedWidth, height: roundedHeight });
        }
    };

    if (!mapImage) {
        return (
            <View style={styles.container} onLayout={onLayout}>
                <View style={styles.placeholder}>
                    <Text style={styles.placeholderText}>No Map Available</Text>
                </View>
            </View>
        );
    }

    if (imageSize.width === 0 || imageSize.height === 0) {
        return (
            <View style={styles.container} onLayout={onLayout}>
                <View style={styles.placeholder}>
                    <Text>Loading Map...</Text>
                </View>
            </View>
        );
    }

    // Calculate displayed dimensions
    const isRotatedSideways = rotation === 90 || rotation === 270;
    const effectiveImageWidth = isRotatedSideways ? imageSize.height : imageSize.width;
    const effectiveImageHeight = isRotatedSideways ? imageSize.width : imageSize.height;

    let displayedWidth = 0;
    let displayedHeight = 0;

    if (containerSize.width > 0 && containerSize.height > 0) {
        const widthRatio = containerSize.width / effectiveImageWidth;
        const heightRatio = containerSize.height / effectiveImageHeight;
        const scale = Math.min(widthRatio, heightRatio);

        displayedWidth = effectiveImageWidth * scale;
        displayedHeight = effectiveImageHeight * scale;
    }

    // Calculate robot position in pixels
    let robotPixelX = 0;
    let robotPixelY = 0;
    let robotRotation = 0;

    if (robotPose) {
        robotPixelX = (robotPose.x - originX) / resolution;
        robotPixelY = imageSize.height - (robotPose.y - originY) / resolution;
        robotRotation = -(robotPose.theta * 180 / Math.PI);
    }

    const handleTouch = (event: any) => {
        if (!onMapPress || !imageSize.width || !containerSize.width || !containerRef.current) return;

        const touch = event.nativeEvent.touches[0] || event.nativeEvent.changedTouches[0];
        if (!touch) return;

        containerRef.current.measure((x, y, width, height, pageX, pageY) => {
            const locationX = touch.pageX - pageX;
            const locationY = touch.pageY - pageY;

            const widthRatio = containerSize.width / effectiveImageWidth;
            const heightRatio = containerSize.height / effectiveImageHeight;
            const scale = Math.min(widthRatio, heightRatio);

            const displayedWidth = effectiveImageWidth * scale;
            const displayedHeight = effectiveImageHeight * scale;

            const offsetX = (containerSize.width - displayedWidth) / 2;
            const offsetY = (containerSize.height - displayedHeight) / 2;

            const pixmapX = locationX - offsetX;
            const pixmapY = locationY - offsetY;

            if (pixmapX < 0 || pixmapX > displayedWidth || pixmapY < 0 || pixmapY > displayedHeight) {
                console.log('Outside bounds');
                return;
            }

            const originalX = (pixmapX / displayedWidth) * imageSize.width;
            const originalY = (pixmapY / displayedHeight) * imageSize.height;

            const worldX = originX + (originalX * resolution);
            const worldY = originY + ((imageSize.height - originalY) * resolution);

            console.log('Map pressed at:', worldX, worldY);
            onMapPress(worldX, worldY);
        });
    };

    return (
        <View
            ref={containerRef}
            style={styles.container}
            onLayout={onLayout}
            onTouchEnd={onMapPress ? handleTouch : undefined}
        >
            <View style={{ width: displayedWidth, height: displayedHeight, transform: [{ rotate: `${rotation}deg` }] }}>
                <View style={{
                    width: isRotatedSideways ? displayedHeight : displayedWidth,
                    height: isRotatedSideways ? displayedWidth : displayedHeight,
                    alignItems: 'center',
                    justifyContent: 'center'
                }}>
                    <View style={{
                        width: imageSize.width,
                        height: imageSize.height,
                        transform: [
                            { scale: displayedWidth / (isRotatedSideways ? imageSize.height : imageSize.width) },
                            { rotate: `${rotation}deg` }
                        ]
                    }}>
                        <Image
                            source={{ uri: mapImage }}
                            style={[StyleSheet.absoluteFill, { width: '100%', height: '100%' }]}
                            resizeMode="contain"
                        />
                        <Svg
                            height="100%"
                            width="100%"
                            viewBox={`0 0 ${imageSize.width} ${imageSize.height}`}
                            style={StyleSheet.absoluteFill}
                        >
                            {/* Path Line */}
                            {robotPose && destination && (
                                <Line
                                    x1={robotPixelX}
                                    y1={robotPixelY}
                                    x2={(destination.x - originX) / resolution}
                                    y2={imageSize.height - (destination.y - originY) / resolution}
                                    stroke="#00FFFF"
                                    strokeWidth="2"
                                    strokeDasharray="5, 5"
                                />
                            )}

                            {/* Custom markers */}
                            {customMarkers.map((marker, index) => {
                                const markerPixelX = (marker.x - originX) / resolution;
                                const markerPixelY = imageSize.height - (marker.y - originY) / resolution;
                                return (
                                    <React.Fragment key={index}>
                                        <Circle
                                            cx={markerPixelX}
                                            cy={markerPixelY}
                                            r={3}
                                            fill={marker.color || '#10b981'}
                                            stroke="#FFFFFF"
                                            strokeWidth="1"
                                        />
                                        {marker.label && (
                                            <SvgText
                                                x={markerPixelX}
                                                y={markerPixelY - 8}
                                                fill={marker.color || '#FFFFFF'}
                                                fontSize="10"
                                                fontWeight="bold"
                                                textAnchor="middle"
                                                stroke="black"
                                                strokeWidth="0.5"
                                            >
                                                {marker.label}
                                            </SvgText>
                                        )}
                                    </React.Fragment>
                                );
                            })}

                            {robotPose && (
                                <G
                                    x={robotPixelX}
                                    y={robotPixelY}
                                    rotation={robotRotation}
                                    origin="0, 0"
                                >
                                    <Polygon
                                        points="-5,-5 10,0 -5,5"
                                        fill="#FF0000"
                                        stroke="#FFFFFF"
                                        strokeWidth="1"
                                        scale="0.7"
                                    />
                                </G>
                            )}
                        </Svg>
                    </View>
                </View>
            </View>
        </View>
    );
};

const styles = StyleSheet.create({
    container: {
        flex: 1,
        justifyContent: 'center',
        alignItems: 'center',
        backgroundColor: '#eee',
        overflow: 'hidden',
        width: '100%',
        height: '100%',
    },
    placeholder: {
        flex: 1,
        justifyContent: 'center',
        alignItems: 'center',
    },
    placeholderText: {
        color: '#666',
    },
});

export default MapDisplay;
