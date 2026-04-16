import React, { useRef } from 'react';
import { View, StyleSheet, PanResponder, Animated } from 'react-native';
import Svg, { Circle } from 'react-native-svg';

interface JoystickProps {
    onMove: (vx: number, vth: number) => void;
    onStop: () => void;
}

const Joystick: React.FC<JoystickProps> = ({ onMove, onStop }) => {
    const pan = useRef(new Animated.ValueXY()).current;
    const radius = 75; // Joystick radius
    const handleRadius = 25;

    const panResponder = useRef(
        PanResponder.create({
            onStartShouldSetPanResponder: () => true,
            onPanResponderMove: (evt, gestureState) => {
                let x = gestureState.dx;
                let y = gestureState.dy;

                // Limit movement to radius
                const distance = Math.sqrt(x * x + y * y);
                if (distance > radius) {
                    const angle = Math.atan2(y, x);
                    x = radius * Math.cos(angle);
                    y = radius * Math.sin(angle);
                }

                pan.setValue({ x, y });

                // Calculate speeds
                const maxLinearSpeed = 0.5;
                const maxAngularSpeed = 1.0;

                let vx = -(y / radius) * maxLinearSpeed;
                let vth = -(x / radius) * maxAngularSpeed;

                // Deadzone / Snap-to-axis logic
                // If mostly horizontal, snap vx to 0 for pure rotation
                if (Math.abs(x) > Math.abs(y) * 2) {
                    vx = 0;
                }
                // If mostly vertical, snap vth to 0 for pure forward/backward
                else if (Math.abs(y) > Math.abs(x) * 2) {
                    vth = 0;
                }

                onMove(vx, vth);
            },
            onPanResponderRelease: () => {
                Animated.spring(pan, {
                    toValue: { x: 0, y: 0 },
                    useNativeDriver: false,
                }).start();
                onStop();
            },
        })
    ).current;

    return (
        <View style={styles.container}>
            <View style={styles.wrapper}>
                <Svg height={radius * 2 + handleRadius * 2} width={radius * 2 + handleRadius * 2} style={styles.bg}>
                    <Circle
                        cx={radius + handleRadius}
                        cy={radius + handleRadius}
                        r={radius}
                        fill="rgba(80, 80, 80, 0.5)"
                    />
                </Svg>
                <Animated.View
                    style={[
                        styles.handle,
                        {
                            transform: pan.getTranslateTransform(),
                        },
                    ]}
                    {...panResponder.panHandlers}
                />
            </View>
        </View>
    );
};

const styles = StyleSheet.create({
    container: {
        alignItems: 'center',
        justifyContent: 'center',
    },
    wrapper: {
        alignItems: 'center',
        justifyContent: 'center',
        width: 200, // radius * 2 + handleRadius * 2
        height: 200,
    },
    bg: {
        position: 'absolute',
    },
    handle: {
        width: 50,
        height: 50,
        borderRadius: 25,
        backgroundColor: '#2A82DA',
    },
});

export default Joystick;
