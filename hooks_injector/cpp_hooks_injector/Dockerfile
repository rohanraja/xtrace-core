# Use Node.js 18 as base image
FROM node:18-alpine

# Install clang-format
RUN apk add --no-cache clang-extra-tools

# Set working directory
WORKDIR /app

# Copy package.json and package-lock.json (if available)
COPY package*.json ./

# Install dependencies
RUN npm install

# Copy source code
COPY . .

# Build the application
RUN npm run build && npm run build-serve

# Expose port 3001
EXPOSE 3001

# Set environment variables
ENV NODE_ENV=production
ENV PORT=3001

# Create non-root user for security
RUN addgroup -g 1001 -S nodejs
RUN adduser -S nodejs -u 1001

# Change ownership of the app directory
RUN chown -R nodejs:nodejs /app
USER nodejs

# Health check
HEALTHCHECK --interval=30s --timeout=3s --start-period=5s --retries=3 \
  CMD node -e "require('http').get('http://localhost:3001/health', (res) => { process.exit(res.statusCode === 200 ? 0 : 1) }).on('error', () => { process.exit(1) })"

# Start the server
CMD ["node", "dist/serve.js"]
