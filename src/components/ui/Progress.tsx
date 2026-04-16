import React from 'react';
import {View, StyleSheet, ViewStyle} from 'react-native';
import {colors, borderRadius} from '../../theme';

interface ProgressProps {
  value: number;
  max?: number;
  height?: number;
  color?: string;
  backgroundColor?: string;
  style?: ViewStyle;
}

export const Progress: React.FC<ProgressProps> = ({
  value,
  max = 100,
  height = 8,
  color = colors.primary.main,
  backgroundColor = colors.primary.main,
  style,
}) => {
  const percentage = Math.min(Math.max((value / max) * 100, 0), 100);

  return (
    <View
      style={[
        styles.container,
        {height, backgroundColor, borderRadius: height / 2},
        style,
      ]}>
      <View
        style={[
          styles.fill,
          {
            width: `${percentage}%`,
            backgroundColor: color,
            borderRadius: height / 2,
          },
        ]}
      />
    </View>
  );
};

const styles = StyleSheet.create({
  container: {
    width: '100%',
    overflow: 'hidden',
  },
  fill: {
    height: '100%',
  },
});

export default Progress;

